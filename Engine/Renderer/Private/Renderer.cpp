// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "Renderer.hpp"
#include "CmdList.hpp"
#include "Vertex.hpp"
#include "RenderQuad.hpp"
#include "CmdList.hpp"
#include "RenderCmd.hpp"
#include "MeshDescriptor.hpp"
#include "MaterialDescriptor.hpp"
#include "UserParams.hpp"
#include "TextureType.hpp"
#include "UIOverlay.hpp"
#include "RendererData.hpp"
#include "MeshData.hpp"
#include "LightDescriptor.hpp"
#include "GlobalDescriptor.hpp"
#include "StructuredBuffer.hpp"
#include "VertexDescription.hpp"
#include "SkyAtmosphere.hpp"
#include "Decal.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/FrameBuffer.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Shader.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Buffer.hpp"

#include "Core/Core.hpp"
#include "Filesystem/Filesystem.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"

#include "UI/UI.hpp"

#include <array>

namespace Recluse {


Renderer& gRenderer() { 
  return Renderer::Instance();
}


Renderer::Renderer()
  : m_pRhi(nullptr)
  , m_Rendering(false)
  , m_Initialized(false)
  , m_pLights(nullptr)
  , m_pGlobal(nullptr)
  , m_AntiAliasing(false)
  , m_pSky(nullptr)
  , m_pSkyboxCmdBuffer(nullptr)
  , m_cpuFence(nullptr)
  , m_SkyboxFinished(nullptr)
  , m_TotalCmdBuffers(3)
  , m_CurrCmdBufferIdx(0)
  , m_Minimized(false)
  , m_pGlobalIllumination(nullptr)
  , m_decalEngine(nullptr)
{
  m_HDR._Enabled = true;
  m_Offscreen._CmdBuffers.resize(m_TotalCmdBuffers);
  m_Offscreen._ShadowCmdBuffers.resize(m_TotalCmdBuffers);

  m_Downscale._Horizontal = 0;
  m_Downscale._Strength = 1.0f;
  m_Downscale._Scale = 1.0f;

  m_cmdDeferredList.Resize(1024);
  m_forwardCmdList.Resize(1024);
  m_uiCmdList.Resize(1024);

  m_cmdDeferredList.SetSortFunc([&](MeshRenderCmd& cmd1, MeshRenderCmd& cmd2) -> bool {
    if (!cmd1._pMeshDesc || !cmd2._pMeshDesc) return false;
    //if (!cmd1._pTarget->_bRenderable || !cmd2._pTarget->_bRenderable) return false;
    MeshDescriptor* mesh1 = cmd1._pMeshDesc;
    MeshDescriptor* mesh2 = cmd2._pMeshDesc;
    Matrix4 m1 = mesh1->ObjectData()->_Model;
    Matrix4 m2 = mesh2->ObjectData()->_Model;

    Vector4 native_pos = m_pGlobal->Data()->_CameraPos;
    Vector3 cam_pos = Vector3(native_pos.x, native_pos.y, native_pos.z);
    Vector3 v1 = Vector3(m1[3][0], m1[3][1], m1[3][2]) - cam_pos;
    Vector3 v2 = Vector3(m2[3][0], m2[3][1], m2[3][2]) - cam_pos;

    return v1.Magnitude() < v2.Magnitude();
  });

  // Use painter's algorithm in this case for forward, simply because of 
  // transparent objects.
  m_forwardCmdList.SetSortFunc([&](MeshRenderCmd& cmd1, MeshRenderCmd& cmd2) -> bool {
    if (!cmd1._pMeshDesc || !cmd2._pMeshDesc) return false;
    //if (!cmd1._pTarget->_bRenderable || !cmd2._pTarget->_bRenderable) return false;
    MeshDescriptor* mesh1 = cmd1._pMeshDesc;
    MeshDescriptor* mesh2 = cmd2._pMeshDesc;
    Matrix4 m1 = mesh1->ObjectData()->_Model;
    Matrix4 m2 = mesh2->ObjectData()->_Model;

    Vector4 native_pos = m_pGlobal->Data()->_CameraPos;
    Vector3 cam_pos = Vector3(native_pos.x, native_pos.y, native_pos.z);
    Vector3 v1 = Vector3(m1[3][0], m1[3][1], m1[3][2]) - cam_pos;
    Vector3 v2 = Vector3(m2[3][0], m2[3][1], m2[3][2]) - cam_pos;

    return v1.Magnitude() > v2.Magnitude();
  });
}


Renderer::~Renderer()
{
}

void Renderer::OnStartUp()
{
  if (!gCore().IsActive()) {
    R_DEBUG(rError, "Core is not active! Start up the core first!\n");
    return;
  }
  VulkanRHI::CreateContext();
  VulkanRHI::FindPhysicalDevice();
  if (!m_pRhi) m_pRhi = new VulkanRHI();
}


void Renderer::OnShutDown()
{
  CleanUp();

  // Shutdown globals.
  VulkanRHI::gPhysicalDevice.CleanUp();
  VulkanRHI::gContext.CleanUp();
  R_DEBUG(rNotify, "Vulkan Renderer successfully cleaned up.\n");
}


void Renderer::BeginFrame()
{
  m_Rendering = true;
  //m_pRhi->PresentWaitIdle();
  m_pRhi->AcquireNextImage();
}


void Renderer::EndFrame()
{
  m_Rendering = false;
  m_pRhi->Present();
}


void Renderer::WaitForCpuFence()
{
  VkFence fence[] = { m_cpuFence->Handle() };
  m_pRhi->WaitForFences(1, fence, VK_TRUE, UINT64_MAX);
  m_pRhi->ResetFences(1, fence);
}


void Renderer::Render()
{
  if (m_Minimized) {
    // Window was minimized, ignore any cpu draw requests and prevent frame rendering
    // until window is back up.
    ClearCmdLists();
    //WaitForCpuFence();
    return;
  }

  // TODO(): Signal a beginning and end callback or so, when performing 
  // any rendering.
  // Update the scene descriptors before rendering the frame.
  SortCmdLists();

  // Wait for fences before starting next frame.
  WaitForCpuFence();

  UpdateSceneDescriptors();
  CheckCmdUpdate();

  // TODO(): Need to clean this up.
  VkCommandBuffer offscreen_CmdBuffers[3] = { 
    m_Offscreen._CmdBuffers[CurrentCmdBufferIdx()]->Handle(), 
    nullptr,
    nullptr
  };

  VkSemaphore offscreen_WaitSemas[] = { m_pRhi->LogicDevice()->ImageAvailableSemaphore() };
  VkSemaphore offscreen_SignalSemas[] = { m_Offscreen._Semaphore->Handle() };

  VkCommandBuffer pbr_CmdBuffers[] = { m_Pbr._CmdBuffer->Handle() };
  VkSemaphore pbr_SignalSemas[] = { m_Pbr._Sema->Handle() };
  VkPipelineStageFlags waitFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };

  VkSubmitInfo offscreenSI = {};
  offscreenSI.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  offscreenSI.pCommandBuffers = offscreen_CmdBuffers;
  offscreenSI.commandBufferCount = 1; // assuming rendering no shadows at first.
  offscreenSI.signalSemaphoreCount = 1;
  offscreenSI.pSignalSemaphores = offscreen_SignalSemas;
  offscreenSI.waitSemaphoreCount = 1;
  offscreenSI.pWaitSemaphores = offscreen_WaitSemas;
  offscreenSI.pWaitDstStageMask = waitFlags;

  // pbr pass, waits for gbuffer, shadow, and sky rendering, to finish.
  VkSubmitInfo pbrSi = { };
  pbrSi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  pbrSi.commandBufferCount = 1;
  pbrSi.pCommandBuffers = pbr_CmdBuffers;
  pbrSi.signalSemaphoreCount = 1;
  pbrSi.waitSemaphoreCount = 1;
  pbrSi.pWaitSemaphores = offscreen_SignalSemas;
  pbrSi.pSignalSemaphores = pbr_SignalSemas;
  pbrSi.pWaitDstStageMask = waitFlags;

  // skybox, waits for pbr to finish rendering.
  VkSubmitInfo skyboxSI = offscreenSI;
  VkSemaphore skybox_SignalSemas[] = { m_SkyboxFinished->Handle() };
  VkCommandBuffer skybox_CmdBuffer = m_pSkyboxCmdBuffer->Handle();
  skyboxSI.commandBufferCount = 1;
  skyboxSI.pCommandBuffers = &skybox_CmdBuffer;
  skyboxSI.pSignalSemaphores = skybox_SignalSemas;
  skyboxSI.pWaitSemaphores = pbr_SignalSemas;

  VkSubmitInfo forwardSi = {};
  VkCommandBuffer forwardBuffers[] = { m_Forward._CmdBuffer->Handle() };
  VkSemaphore forwardSignalSemas[] = { m_Forward._Semaphore->Handle() };
  forwardSi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  forwardSi.commandBufferCount = 1;
  forwardSi.pCommandBuffers = forwardBuffers;
  forwardSi.signalSemaphoreCount = 1;
  forwardSi.waitSemaphoreCount = 1;
  forwardSi.pWaitSemaphores = skybox_SignalSemas;
  forwardSi.pSignalSemaphores = forwardSignalSemas;
  forwardSi.pWaitDstStageMask = waitFlags;

  // Postprocessing, HDR waits for skybox to finish rendering onto scene.
  VkSubmitInfo hdrSI = offscreenSI;
  VkSemaphore hdr_SignalSemas[] = { m_HDR._Semaphore->Handle() };
  VkCommandBuffer hdrCmd = m_HDR._CmdBuffer->Handle();
  hdrSI.pCommandBuffers = &hdrCmd;
  hdrSI.pSignalSemaphores = hdr_SignalSemas;
  hdrSI.pWaitSemaphores = forwardSignalSemas;

  VkSemaphore* final_WaitSemas = hdr_SignalSemas;
  if (!m_HDR._Enabled) final_WaitSemas = forwardSignalSemas;

  VkSubmitInfo finalSi = { };
  finalSi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkCommandBuffer finalCmdBuffer = { m_pFinalCommandBuffer->Handle() };
  VkSemaphore finalFinished = { m_pFinalFinished->Handle() };
  finalSi.commandBufferCount = 1;
  finalSi.pCommandBuffers = &finalCmdBuffer;
  finalSi.pSignalSemaphores = &finalFinished;
  finalSi.pWaitDstStageMask = waitFlags;
  finalSi.signalSemaphoreCount = 1;
  finalSi.waitSemaphoreCount = 1;
  finalSi.pWaitSemaphores = final_WaitSemas;

  VkSubmitInfo uiSi = { };
  uiSi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkCommandBuffer uiCmdBuffer = { m_pUI->GetCommandBuffer()->Handle() };
  VkSemaphore uiSignalSema = { m_pUI->GetSemaphore()->Handle() };
  uiSi.commandBufferCount = 1;
  uiSi.pCommandBuffers = &uiCmdBuffer;
  uiSi.waitSemaphoreCount = 1;
  uiSi.pWaitSemaphores = &finalFinished;
  uiSi.signalSemaphoreCount = 1;
  uiSi.pSignalSemaphores = &uiSignalSema;
  uiSi.pWaitDstStageMask = waitFlags;

  // begin frame. This is where we start our render process per frame.
  BeginFrame();
    while (m_Offscreen._CmdBuffers[CurrentCmdBufferIdx()]->Recording() || !m_pRhi->CmdBuffersComplete()) {}

    // Render shadow map here. Primary shadow map is our concern.
    if (m_pLights->PrimaryShadowEnabled()) {
      offscreen_CmdBuffers[offscreenSI.commandBufferCount] = m_Offscreen._ShadowCmdBuffers[CurrentCmdBufferIdx()]->Handle();
      offscreenSI.commandBufferCount += 1; // Add shadow buffer to render.
    }

    // Check if sky needs to update it's cubemap.
    if (m_pSky->NeedsRendering()) {
      offscreen_CmdBuffers[offscreenSI.commandBufferCount] = m_pSky->CmdBuffer()->Handle();
      offscreenSI.commandBufferCount += 1; // Add sky render buffer.
      m_pSky->MarkClean();
    }

    VkSubmitInfo submits[] { offscreenSI, pbrSi, skyboxSI, forwardSi };
    // Submit to renderqueue.
    m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 4, submits);

    // High Dynamic Range and Gamma Pass. Post process after rendering. This will include
    // Bloom, AA, other effects.
    if (m_HDR._Enabled) { m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 1, &hdrSI); }

    //
    // TODO(): Add antialiasing here.
    // 

    // Final render after post process.
    m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 1, &finalSi);

    // Before calling this cmd buffer, we want to submit our offscreen buffer first, then
    // sent our signal to our swapchain cmd buffers.
    
    // Render the Overlay.
    m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 1, &uiSi);

    // Signal graphics finished on the final output.
    VkSemaphore signal = m_pRhi->GraphicsFinishedSemaphore();
    m_pRhi->SubmitCurrSwapchainCmdBuffer(1, &uiSignalSema, 1, &signal, m_cpuFence->Handle()); // cpuFence will need to wait until overlay is finished.
  EndFrame();


  // Compute pipeline render.
  VkSubmitInfo computeSubmit = { };
  computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  computeSubmit.commandBufferCount = 0;
  computeSubmit.pCommandBuffers = nullptr;
  computeSubmit.signalSemaphoreCount = 0;
  computeSubmit.waitSemaphoreCount = 0;

  m_pRhi->ComputeSubmit(DEFAULT_QUEUE_IDX, computeSubmit);

  // Clear command list afterwards.
  ClearCmdLists();
}


void Renderer::CleanUp()
{
  // Must wait for all command buffers to finish before cleaning up.
  m_pRhi->DeviceWaitIdle();

  // We properly want to use smart ptrs...
  m_pGlobal->CleanUp();
  delete m_pGlobal;
  m_pGlobal = nullptr;

  m_pLights->CleanUp();
  delete m_pLights;
  m_pLights = nullptr;
  
  CleanUpSkybox();
  m_pSky->CleanUp();
  delete m_pSky;
  m_pSky = nullptr;

  if (m_pUI) {
    m_pUI->CleanUp();
    delete m_pUI;
    m_pUI = nullptr;
  }

  CleanUpGlobalIlluminationBuffer();

  m_RenderQuad.CleanUp();
  CleanUpForwardPBR();
  CleanUpPBR();
  CleanUpHDR(true);
  CleanUpDownscale(true);
  CleanUpOffscreen(true);
  CleanUpFinalOutputs();
  CleanUpDescriptorSetLayouts();
  CleanUpGraphicsPipelines();
  CleanUpFrameBuffers();
  CleanUpRenderTextures(true);

  m_pRhi->FreeVkFence(m_cpuFence);
  m_cpuFence = nullptr;

  if (m_pRhi) {
    m_pRhi->CleanUp();
    delete m_pRhi;
    m_pRhi = nullptr;
  }
  m_Initialized = false;
}


b32 Renderer::Initialize(Window* window, const GraphicsConfigParams* params)
{
  if (!window) return false;
  if (m_Initialized) return true;

  if (!params) {
    params = &kDefaultGpuConfigs;
  }
  
  m_pWindow = window;
  m_pRhi->Initialize(window->Handle(), params);

  SetUpRenderTextures(true);
  SetUpFrameBuffers();
  SetUpDescriptorSetLayouts();
  m_RenderQuad.Initialize(m_pRhi);

  GlobalDescriptor* gMat = new GlobalDescriptor();
  gMat->m_pRhi = m_pRhi;
  gMat->Initialize();
  gMat->Update();
  m_pGlobal = gMat;

  m_pSky = new SkyRenderer();
  m_pSky->Initialize();
  m_pSky->MarkDirty();

  m_pLights = new LightDescriptor();
  m_pLights->m_pRhi = m_pRhi;
  m_pLights->Initialize(params->_Shadows);
  m_pLights->Update();

  SetUpSkybox();
  SetUpGraphicsPipelines();
  SetUpFinalOutputs();
  SetUpOffscreen(true);
  SetUpDownscale(true);
  SetUpHDR(true);
  SetUpPBR();
  SetUpForwardPBR();

  VkFenceCreateInfo fenceCi = { };
  fenceCi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  
  m_cpuFence = m_pRhi->CreateVkFence();
  m_cpuFence->Initialize(fenceCi);

  m_pRhi->SetSwapchainCmdBufferBuild([&] (CommandBuffer& cmdBuffer, VkRenderPassBeginInfo& defaultRenderpass) -> void {
    // Do stuff with the buffer.
    VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
    VkViewport viewport = { };
    viewport.height = (r32) windowExtent.height;
    viewport.width = (r32) windowExtent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    
    GraphicsPipeline* finalPipeline = output_pipelineKey;
    DescriptorSet* finalSet = output_descSetKey;
    
    cmdBuffer.BeginRenderPass(defaultRenderpass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer.SetViewPorts(0, 1, &viewport);
      cmdBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Pipeline());
      VkDescriptorSet finalDescriptorSets[] = { finalSet->Handle() };    

      cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Layout(), 0, 1, finalDescriptorSets, 0, nullptr);
      VkBuffer vertexBuffer = m_RenderQuad.Quad()->Handle()->NativeBuffer();
      VkBuffer indexBuffer = m_RenderQuad.Indices()->Handle()->NativeBuffer();
      VkDeviceSize offsets[] = { 0 };

      cmdBuffer.BindIndexBuffer(indexBuffer, 0, GetNativeIndexType(m_RenderQuad.Indices()->GetSizeType()));
      cmdBuffer.BindVertexBuffers(0, 1, &vertexBuffer, offsets);

      cmdBuffer.DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer.EndRenderPass();
  });

  UpdateRuntimeConfigs(params);

  if (!m_pUI) {
    m_pUI = new UIOverlay();
    m_pUI->Initialize(m_pRhi);
  }

  SetUpGlobalIlluminationBuffer();

  m_Initialized = true;
  return true;
}


void Renderer::SetUpDescriptorSetLayouts()
{
  // Light space.
  {
    std::array<VkDescriptorSetLayoutBinding, 1> LightViewBindings;
    LightViewBindings[0].pImmutableSamplers = nullptr;
    LightViewBindings[0].binding = 0;
    LightViewBindings[0].descriptorCount = 1;
    LightViewBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    LightViewBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    DescriptorSetLayout* LightViewLayout = m_pRhi->CreateDescriptorSetLayout();
    VkDescriptorSetLayoutCreateInfo LightViewInfo = { };
    LightViewInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LightViewInfo.bindingCount = static_cast<u32>(LightViewBindings.size());
    LightViewInfo.pBindings = LightViewBindings.data();
    LightViewLayout->Initialize(LightViewInfo);

    LightViewDescriptorSetLayoutKey = LightViewLayout;  
  }

  DescriptorSetLayout* GlobalSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* MeshSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* MaterialSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* LightSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* BonesSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* SkySetLayout = m_pRhi->CreateDescriptorSetLayout();
  GlobalSetLayoutKey = GlobalSetLayout;
  MeshSetLayoutKey =  MeshSetLayout;
  MaterialSetLayoutKey = MaterialSetLayout;
  LightSetLayoutKey = LightSetLayout;
  BonesSetLayoutKey = BonesSetLayout;
  skybox_setLayoutKey = SkySetLayout;

  // Global and Mesh Layout.
  {
    std::array<VkDescriptorSetLayoutBinding, 1> GlobalBindings;
    GlobalBindings[0].binding = 0;
    GlobalBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    GlobalBindings[0].descriptorCount = 1;
    GlobalBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    GlobalBindings[0].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo GlobalLayout = {};
    GlobalLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    GlobalLayout.bindingCount = static_cast<u32>(GlobalBindings.size());
    GlobalLayout.pBindings = GlobalBindings.data();
    GlobalSetLayout->Initialize(GlobalLayout);
    MeshSetLayout->Initialize(GlobalLayout);
  }

  // MaterialDescriptor Layout.
  {
    std::array<VkDescriptorSetLayoutBinding, 6> MaterialBindings;
    // MaterialDescriptor Buffer
    MaterialBindings[5].binding = 0;
    MaterialBindings[5].descriptorCount = 1;
    MaterialBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    MaterialBindings[5].pImmutableSamplers = nullptr;
    MaterialBindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // albedo
    MaterialBindings[0].binding = 1;
    MaterialBindings[0].descriptorCount = 1;
    MaterialBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[0].pImmutableSamplers = nullptr;
    MaterialBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // roughness metallic
    MaterialBindings[1].binding = 2;
    MaterialBindings[1].descriptorCount = 1;
    MaterialBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[1].pImmutableSamplers = nullptr;
    MaterialBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // normal
    MaterialBindings[2].binding = 3;
    MaterialBindings[2].descriptorCount = 1;
    MaterialBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[2].pImmutableSamplers = nullptr;
    MaterialBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // ao
    MaterialBindings[3].binding = 4;
    MaterialBindings[3].descriptorCount = 1;
    MaterialBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[3].pImmutableSamplers = nullptr;
    MaterialBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // emissive
    MaterialBindings[4].binding = 5;
    MaterialBindings[4].descriptorCount = 1;
    MaterialBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[4].pImmutableSamplers = nullptr;
    MaterialBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo MaterialLayout = {};
    MaterialLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    MaterialLayout.bindingCount = static_cast<u32>(MaterialBindings.size());
    MaterialLayout.pBindings = MaterialBindings.data();

    MaterialSetLayout->Initialize(MaterialLayout);
  }

  // PBR descriptor layout.
  {
    DescriptorSetLayout* pbr_Layout = m_pRhi->CreateDescriptorSetLayout();
    pbr_DescLayoutKey = pbr_Layout;
    std::array<VkDescriptorSetLayoutBinding, 5> bindings;

    // Albedo
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].pImmutableSamplers = nullptr;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;  

    // Normal
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].pImmutableSamplers = nullptr;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;  

    // Position
    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].pImmutableSamplers = nullptr;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Emission-Roughness-Metallic
    bindings[3].binding = 3;
    bindings[3].descriptorCount = 1;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].pImmutableSamplers = nullptr;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Depth
    bindings[4].binding = 4;
    bindings[4].descriptorCount = 1;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].pImmutableSamplers = nullptr;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo PbrLayout = { };
    PbrLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    PbrLayout.bindingCount = static_cast<u32>(bindings.size());
    PbrLayout.pBindings = bindings.data();
    pbr_Layout->Initialize(PbrLayout);
  }

  // Bones Layout.
  {
    // bones.
    std::array<VkDescriptorSetLayoutBinding, 1> BonesBindings;
    BonesBindings[0].binding = 0;
    BonesBindings[0].descriptorCount = 1;
    BonesBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    BonesBindings[0].pImmutableSamplers = nullptr;
    BonesBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutCreateInfo BoneLayout = { };
    BoneLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    BoneLayout.bindingCount = static_cast<u32>(BonesBindings.size());
    BoneLayout.pBindings = BonesBindings.data();
    BonesSetLayout->Initialize(BoneLayout);
  }

  // Skybox samplerCube.
  {
    VkDescriptorSetLayoutBinding skyboxBind = { };
    skyboxBind.binding = 0;
    skyboxBind.descriptorCount = 1;
    skyboxBind.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyboxBind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    skyboxBind.pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutCreateInfo info = { };
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.flags = 0;
    info.bindingCount = 1;
    info.pBindings = &skyboxBind;
    info.pNext = nullptr;
    SkySetLayout->Initialize(info);
  }

  // Global Illumination reflection probe layout.
  {
    DescriptorSetLayout* globalIllumLayout = m_pRhi->CreateDescriptorSetLayout();
    globalIllumination_DescNoLR = m_pRhi->CreateDescriptorSetLayout();
    globalIllumination_DescLR = globalIllumLayout;
    
    std::array<VkDescriptorSetLayoutBinding, 4> globalIllum;
    // Global IrrMap.
    globalIllum[0].binding = 0;
    globalIllum[0].descriptorCount = 1;
    globalIllum[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globalIllum[0].pImmutableSamplers = nullptr;
    globalIllum[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Global EnvMap.
    globalIllum[1].binding = 1;
    globalIllum[1].descriptorCount = 1;
    globalIllum[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globalIllum[1].pImmutableSamplers = nullptr;
    globalIllum[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Irradiance Map array.
    globalIllum[2].binding = 2;
    globalIllum[2].descriptorCount = 1;
    globalIllum[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globalIllum[2].pImmutableSamplers = nullptr;
    globalIllum[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Radiance (Enviroment Map) array.
    globalIllum[3].binding = 3;
    globalIllum[3].descriptorCount = 1;
    globalIllum[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globalIllum[3].pImmutableSamplers = nullptr;
    globalIllum[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.flags = 0;
    info.bindingCount = static_cast<u32>(globalIllum.size());
    info.pBindings = globalIllum.data();
    info.pNext = nullptr;
    globalIllumLayout->Initialize(info);
    info.bindingCount = static_cast<u32>(globalIllum.size() - 2);
    globalIllumination_DescNoLR->Initialize(info);

    
  }

  // Light layout.
  {
    std::array<VkDescriptorSetLayoutBinding, 2> LightBindings;
    LightBindings[0].binding = 0;
    LightBindings[0].descriptorCount = 1;
    LightBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    LightBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    LightBindings[0].pImmutableSamplers = nullptr;

    LightBindings[1].binding = 1;
    LightBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    LightBindings[1].descriptorCount = 1;
    LightBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    LightBindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo LightLayout = { };
    LightLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LightLayout.bindingCount = static_cast<u32>(LightBindings.size());
    LightLayout.pBindings = LightBindings.data();

    LightSetLayout->Initialize(LightLayout);
  }
  // Final/Output Layout pass.
  {
    DescriptorSetLayout* finalSetLayout = m_pRhi->CreateDescriptorSetLayout();
    final_DescSetLayoutKey = finalSetLayout;
  
    std::array<VkDescriptorSetLayoutBinding, 1> finalBindings;

    finalBindings[0].binding = 0;
    finalBindings[0].descriptorCount = 1;
    finalBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    finalBindings[0].pImmutableSamplers = nullptr;
    finalBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo finalLayoutInfo = {};
    finalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    finalLayoutInfo.bindingCount = static_cast<u32>(finalBindings.size());
    finalLayoutInfo.pBindings = finalBindings.data();

    finalSetLayout->Initialize(finalLayoutInfo);
  }
  // HDR Layout pass.
  DescriptorSetLayout* hdrSetLayout = m_pRhi->CreateDescriptorSetLayout();
  hdr_gamma_descSetLayoutKey = hdrSetLayout;
  std::array<VkDescriptorSetLayoutBinding, 3> hdrBindings;
  hdrBindings[0].binding = 0;
  hdrBindings[0].descriptorCount = 1;
  hdrBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  hdrBindings[0].pImmutableSamplers = nullptr;
  hdrBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  
  hdrBindings[1].binding = 1;
  hdrBindings[1].descriptorCount = 1;
  hdrBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  hdrBindings[1].pImmutableSamplers = nullptr;
  hdrBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  hdrBindings[2].binding = 2;
  hdrBindings[2].descriptorCount = 1;
  hdrBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  hdrBindings[2].pImmutableSamplers = nullptr;
  hdrBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo hdrLayoutCi = {};
  hdrLayoutCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  hdrLayoutCi.bindingCount = static_cast<u32>(hdrBindings.size());
  hdrLayoutCi.pBindings = hdrBindings.data();
  
  hdrSetLayout->Initialize(hdrLayoutCi);

  // Downscale descriptor set layout info.
  DescriptorSetLayout* downscaleLayout = m_pRhi->CreateDescriptorSetLayout();
  DownscaleBlurLayoutKey = downscaleLayout;

  VkDescriptorSetLayoutBinding dwnscl[1];
  dwnscl[0].binding = 0;
  dwnscl[0].descriptorCount = 1;
  dwnscl[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  dwnscl[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  dwnscl[0].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo dwnLayout = { };
  dwnLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dwnLayout.bindingCount = 1;
  dwnLayout.pBindings = dwnscl;
  dwnLayout.flags = 0;
  dwnLayout.pNext = nullptr;
  
  downscaleLayout->Initialize(dwnLayout);

  DescriptorSetLayout* GlowLayout = m_pRhi->CreateDescriptorSetLayout();
  GlowDescriptorSetLayoutKey = GlowLayout;
  std::array<VkDescriptorSetLayoutBinding, 4> glow;

  glow[0].binding = 0;
  glow[0].descriptorCount = 1;
  glow[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  glow[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  glow[0].pImmutableSamplers = nullptr;

  glow[1].binding = 1;
  glow[1].descriptorCount = 1;
  glow[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  glow[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  glow[1].pImmutableSamplers = nullptr;

  glow[2].binding = 2;
  glow[2].descriptorCount = 1;
  glow[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  glow[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  glow[2].pImmutableSamplers = nullptr;

  glow[3].binding = 3;
  glow[3].descriptorCount = 1;
  glow[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  glow[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  glow[3].pImmutableSamplers = nullptr;

  dwnLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dwnLayout.bindingCount = static_cast<u32>(glow.size());
  dwnLayout.pBindings = glow.data();
  dwnLayout.flags = 0;
  dwnLayout.pNext = nullptr;

  GlowLayout->Initialize(dwnLayout);
}


void Renderer::CleanUpDescriptorSetLayouts()
{
  m_pRhi->FreeDescriptorSetLayout(GlobalSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(MeshSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(MaterialSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(LightSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(BonesSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(skybox_setLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(LightViewDescriptorSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(final_DescSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(hdr_gamma_descSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(DownscaleBlurLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(GlowDescriptorSetLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(pbr_DescLayoutKey);
  m_pRhi->FreeDescriptorSetLayout(globalIllumination_DescLR);
  m_pRhi->FreeDescriptorSetLayout(globalIllumination_DescNoLR);
}


void Renderer::SetUpFrameBuffers()
{
  VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
  Texture* gbuffer_Albedo = gbuffer_AlbedoAttachKey;
  Texture* gbuffer_Normal = gbuffer_NormalAttachKey;
  Texture* gbuffer_Position = gbuffer_PositionAttachKey;
  Texture* gbuffer_Emission = gbuffer_EmissionAttachKey;
  Texture* gbuffer_Depth = gbuffer_DepthAttachKey;

  FrameBuffer* gbuffer_FrameBuffer = m_pRhi->CreateFrameBuffer();
  gbuffer_FrameBufferKey = gbuffer_FrameBuffer;

  FrameBuffer* pbr_FrameBuffer = m_pRhi->CreateFrameBuffer();
  pbr_FrameBufferKey = pbr_FrameBuffer;

  FrameBuffer* hdrFrameBuffer = m_pRhi->CreateFrameBuffer();
  hdr_gamma_frameBufferKey =  hdrFrameBuffer;

  final_frameBufferKey = m_pRhi->CreateFrameBuffer();

  // Final framebuffer.
  {
    std::array<VkAttachmentDescription, 1> attachmentDescriptions;
    VkSubpassDependency dependencies[2];
    attachmentDescriptions[0] = CreateAttachmentDescription(
      final_renderTargetKey->Format(),
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      final_renderTargetKey->Samples()
    );

    dependencies[0] = CreateSubPassDependency(
      VK_SUBPASS_EXTERNAL,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      0,
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_DEPENDENCY_BY_REGION_BIT
    );

    dependencies[1] = CreateSubPassDependency(
      0,
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_SUBPASS_EXTERNAL,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      VK_DEPENDENCY_BY_REGION_BIT
    );
    
    std::array<VkAttachmentReference, 1> attachmentColors;
    attachmentColors[0].attachment = 0;
    attachmentColors[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<u32>(attachmentColors.size());
    subpass.pColorAttachments = attachmentColors.data();
    subpass.pDepthStencilAttachment = nullptr;

    VkRenderPassCreateInfo renderpassCI = CreateRenderPassInfo(
      static_cast<u32>(attachmentDescriptions.size()),
      attachmentDescriptions.data(),
      2,
      dependencies,
      1,
      &subpass
    );

    std::array<VkImageView, 1> attachments;
    attachments[0] = final_renderTargetKey->View();
    VkFramebufferCreateInfo framebufferCI = CreateFrameBufferInfo(
      windowExtent.width,
      windowExtent.height,
      nullptr, // Finalize() call handles this for us.
      static_cast<u32>(attachments.size()),
      attachments.data(),
      1
    );
    final_renderPass = m_pRhi->CreateRenderPass();
    final_renderPass->Initialize(renderpassCI);
    final_frameBufferKey->Finalize(framebufferCI, final_renderPass);
  }

  std::array<VkAttachmentDescription, 5> attachmentDescriptions;
  VkSubpassDependency dependencies[2];

  attachmentDescriptions[0] = CreateAttachmentDescription(
    gbuffer_Albedo->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Albedo->Samples()
  );

  attachmentDescriptions[1] = CreateAttachmentDescription(
    gbuffer_Normal->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Normal->Samples()
  );

  attachmentDescriptions[2] = CreateAttachmentDescription(
    gbuffer_Position->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Position->Samples()
  );

  attachmentDescriptions[3] = CreateAttachmentDescription(
    gbuffer_Emission->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Emission->Samples()
  );

  attachmentDescriptions[4] = CreateAttachmentDescription(
    gbuffer_Depth->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Depth->Samples()
  );

  dependencies[0] = CreateSubPassDependency(
    VK_SUBPASS_EXTERNAL, 
    VK_ACCESS_MEMORY_READ_BIT, 
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    0, 
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
    VK_DEPENDENCY_BY_REGION_BIT
  );

  dependencies[1] = CreateSubPassDependency(
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VK_DEPENDENCY_BY_REGION_BIT
  );

  std::array<VkAttachmentReference, 4> attachmentColors;
  VkAttachmentReference attachmentDepthRef = { static_cast<u32>(attachmentColors.size()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
  attachmentColors[0].attachment = 0;
  attachmentColors[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[1].attachment = 1;
  attachmentColors[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[2].attachment = 2;
  attachmentColors[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[3].attachment = 3;
  attachmentColors[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = { };
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<u32>(attachmentColors.size());
  subpass.pColorAttachments = attachmentColors.data();
  subpass.pDepthStencilAttachment = &attachmentDepthRef;
  
  VkRenderPassCreateInfo renderpassCI = CreateRenderPassInfo(
    static_cast<u32>(attachmentDescriptions.size()),
    attachmentDescriptions.data(),
    2,
    dependencies,
    1,
    &subpass
  );

  std::array<VkImageView, 5> attachments;
  attachments[0] = gbuffer_Albedo->View();
  attachments[1] = gbuffer_Normal->View();
  attachments[2] = gbuffer_Position->View();
  attachments[3] = gbuffer_Emission->View();
  attachments[4] = gbuffer_Depth->View();

  VkFramebufferCreateInfo framebufferCI = CreateFrameBufferInfo(
    windowExtent.width,
    windowExtent.height,
    nullptr, // Finalize() call handles this for us.
    static_cast<u32>(attachments.size()),
    attachments.data(),
    1
  );

  gbuffer_renderPass = m_pRhi->CreateRenderPass();
  gbuffer_renderPass->Initialize(renderpassCI);
  gbuffer_FrameBuffer->Finalize(framebufferCI, gbuffer_renderPass);

  // pbr framebuffer.
  {
    Texture* pbr_Bright = pbr_BrightTextureKey;
    Texture* pbr_Final = pbr_FinalTextureKey;
    std::array<VkAttachmentDescription, 3> pbrAttachmentDescriptions;
    pbrAttachmentDescriptions[0] = CreateAttachmentDescription(
      pbr_Final->Format(),
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
     pbr_Final->Samples()
    );

    pbrAttachmentDescriptions[1] = CreateAttachmentDescription(
      pbr_Bright->Format(),
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      pbr_Bright->Samples()
    );

    pbrAttachmentDescriptions[2] = CreateAttachmentDescription(
      gbuffer_Depth->Format(),
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Depth->Samples()
    );

    VkSubpassDescription pbrSubpass = {};
    pbrSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    pbrSubpass.colorAttachmentCount = static_cast<u32>(pbrAttachmentDescriptions.size() - 1);
    pbrSubpass.pColorAttachments = attachmentColors.data();
    attachmentDepthRef.attachment = 2;
    pbrSubpass.pDepthStencilAttachment = &attachmentDepthRef;

    VkRenderPassCreateInfo pbrRenderpassCI = CreateRenderPassInfo(
      static_cast<u32>(pbrAttachmentDescriptions.size()),
      pbrAttachmentDescriptions.data(),
      2,
      dependencies,
      1,
      &pbrSubpass
    );

    std::array<VkImageView, 3> pbrAttachments;
    pbrAttachments[0] = pbr_Final->View();
    pbrAttachments[1] = pbr_Bright->View();
    pbrAttachments[2] = gbuffer_Depth->View();

    VkFramebufferCreateInfo pbrFramebufferCI = CreateFrameBufferInfo(
      windowExtent.width,
      windowExtent.height,
      nullptr, // Finalize() call handles this for us.
      static_cast<u32>(pbrAttachments.size()),
      pbrAttachments.data(),
      1
    );

    pbr_renderPass = m_pRhi->CreateRenderPass();
    pbr_renderPass->Initialize(pbrRenderpassCI);
    pbr_FrameBuffer->Finalize(pbrFramebufferCI, pbr_renderPass);

    // Forward renderpass portion.
    pbrAttachmentDescriptions[0] = CreateAttachmentDescription(
      pbr_Final->Format(),
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      pbr_Final->Samples()
    );

    pbrAttachmentDescriptions[1] = CreateAttachmentDescription(
      pbr_Bright->Format(),
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      pbr_Bright->Samples()
    );

    pbrAttachmentDescriptions[2] = CreateAttachmentDescription(
      gbuffer_Depth->Format(),
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Depth->Samples()
    );

    pbr_forwardRenderPass = m_pRhi->CreateRenderPass();
    pbr_forwardRenderPass->Initialize(pbrRenderpassCI);
  }
  
  // No need to render any depth, as we are only writing on a 2d surface.
  Texture* hdrColor = hdr_gamma_colorAttachKey;
  subpass.pDepthStencilAttachment = nullptr;
  attachments[0] = hdrColor->View();
  framebufferCI.attachmentCount = 1;
  attachmentDescriptions[0].format = hdrColor->Format();
  attachmentDescriptions[0].samples = hdrColor->Samples();
  renderpassCI.attachmentCount = 1;
  subpass.colorAttachmentCount = 1;
  hdr_renderPass = m_pRhi->CreateRenderPass();
  hdr_renderPass->Initialize(renderpassCI);
  hdrFrameBuffer->Finalize(framebufferCI, hdr_renderPass);

  // Downscale render textures.
  Texture* rtDownScale2x = RenderTarget2xHorizKey;
  Texture* RenderTarget2xFinal = RenderTarget2xFinalKey;
  Texture* rtDownScale4x = RenderTarget4xScaledKey;
  Texture* RenderTarget4xFinal = RenderTarget4xFinalKey;
  Texture* rtDownScale8x = RenderTarget8xScaledKey;
  Texture* RenderTarget8xFinal = RenderTarget8xFinalKey;
  Texture* rtDownScale16x = RenderTarget16xScaledKey;
  Texture* RenderTarget16xFinal = RenderTarget16xFinalKey;
  Texture* GlowTarget = RenderTargetGlowKey;

  FrameBuffer* DownScaleFB2x = m_pRhi->CreateFrameBuffer();
  FrameBuffer* FB2xFinal = m_pRhi->CreateFrameBuffer();
  FrameBuffer* DownScaleFB4x = m_pRhi->CreateFrameBuffer();
  FrameBuffer* FB4xFinal = m_pRhi->CreateFrameBuffer();
  FrameBuffer* DownScaleFB8x = m_pRhi->CreateFrameBuffer();
  FrameBuffer* FB8xFinal = m_pRhi->CreateFrameBuffer();
  FrameBuffer* DownScaleFB16x = m_pRhi->CreateFrameBuffer();
  FrameBuffer* FB16xFinal = m_pRhi->CreateFrameBuffer();
  FrameBuffer* GlowFB = m_pRhi->CreateFrameBuffer();
  FrameBuffer2xHorizKey = DownScaleFB2x;
  FrameBuffer2xFinalKey = FB2xFinal;
  FrameBuffer4xKey = DownScaleFB4x; 
  FrameBuffer4xFinalKey = FB4xFinal;
  FrameBuffer8xKey = DownScaleFB8x;
  FrameBuffer8xFinalKey = FB8xFinal;
  FrameBuffer16xKey = DownScaleFB16x;
  FrameBuffer16xFinalKey = FB16xFinal;
  FrameBufferGlowKey = GlowFB;

  // 2x
  attachments[0] = RenderTarget2xFinal->View();
  attachmentDescriptions[0].format = RenderTarget2xFinal->Format();
  attachmentDescriptions[0].samples = RenderTarget2xFinal->Samples();
  framebufferCI.width = RenderTarget2xFinal->Width();
  framebufferCI.height = RenderTarget2xFinal->Height();
  FB2xFinal->Finalize(framebufferCI, hdr_renderPass);

  attachments[0] = rtDownScale2x->View();
  attachmentDescriptions[0].format = rtDownScale2x->Format();
  attachmentDescriptions[0].samples = rtDownScale2x->Samples();
  framebufferCI.width = rtDownScale2x->Width();
  framebufferCI.height = rtDownScale2x->Height();
  DownScaleFB2x->Finalize(framebufferCI, hdr_renderPass);

  // 4x
  attachments[0] = RenderTarget4xFinal->View();
  attachmentDescriptions[0].format = RenderTarget4xFinal->Format();
  attachmentDescriptions[0].samples = RenderTarget4xFinal->Samples();
  framebufferCI.width = RenderTarget4xFinal->Width();
  framebufferCI.height = RenderTarget4xFinal->Height();
  FB4xFinal->Finalize(framebufferCI, hdr_renderPass);

  attachments[0] = rtDownScale4x->View();
  attachmentDescriptions[0].format = rtDownScale4x->Format();
  attachmentDescriptions[0].samples = rtDownScale4x->Samples();
  framebufferCI.width = rtDownScale4x->Width();
  framebufferCI.height = rtDownScale4x->Height();
  DownScaleFB4x->Finalize(framebufferCI, hdr_renderPass);

  // 8x
  attachments[0] = RenderTarget8xFinal->View();
  attachmentDescriptions[0].format = RenderTarget8xFinal->Format();
  attachmentDescriptions[0].samples = RenderTarget8xFinal->Samples();
  framebufferCI.width = RenderTarget8xFinal->Width();
  framebufferCI.height = RenderTarget8xFinal->Height();
  FB8xFinal->Finalize(framebufferCI, hdr_renderPass);

  attachments[0] = rtDownScale8x->View();
  attachmentDescriptions[0].format = rtDownScale8x->Format();
  attachmentDescriptions[0].samples = rtDownScale8x->Samples();
  framebufferCI.width = rtDownScale8x->Width();
  framebufferCI.height = rtDownScale8x->Height();
  DownScaleFB8x->Finalize(framebufferCI, hdr_renderPass);

  // 16x
  attachments[0] = RenderTarget16xFinal->View();
  attachmentDescriptions[0].format = RenderTarget16xFinal->Format();
  attachmentDescriptions[0].samples = RenderTarget16xFinal->Samples();
  framebufferCI.width = RenderTarget16xFinal->Width();
  framebufferCI.height = RenderTarget16xFinal->Height();
  FB16xFinal->Finalize(framebufferCI, hdr_renderPass);

  attachments[0] = rtDownScale16x->View();
  attachmentDescriptions[0].format = rtDownScale16x->Format();
  attachmentDescriptions[0].samples = rtDownScale16x->Samples();
  framebufferCI.width = rtDownScale16x->Width();
  framebufferCI.height = rtDownScale16x->Height();
  DownScaleFB16x->Finalize(framebufferCI, hdr_renderPass);


  // Glow
  attachments[0] = GlowTarget->View();
  attachmentDescriptions[0].format = GlowTarget->Format();
  attachmentDescriptions[0].samples = GlowTarget->Samples();
  framebufferCI.width = m_pRhi->SwapchainObject()->SwapchainExtent().width;
  framebufferCI.height = m_pRhi->SwapchainObject()->SwapchainExtent().height;
  GlowFB->Finalize(framebufferCI, hdr_renderPass);
}


void Renderer::SetUpGraphicsPipelines()
{
  VkPipelineInputAssemblyStateCreateInfo assemblyCI = { };
  assemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  assemblyCI.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = { };
  VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.height = static_cast<r32>(windowExtent.height);
  viewport.width = static_cast<r32>(windowExtent.width);

  VkRect2D scissor = { };
  scissor.extent = m_pRhi->SwapchainObject()->SwapchainExtent();
  scissor.offset = { 0, 0 };

  VkPipelineViewportStateCreateInfo viewportCI = { };
  viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportCI.viewportCount = 1;
  viewportCI.pViewports = &viewport;
  viewportCI.scissorCount = 1;
  viewportCI.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizerCI = CreateRasterInfo(
     VK_POLYGON_MODE_FILL,
      VK_FALSE, 
      GBUFFER_CULL_MODE,
      GBUFFER_WINDING_ORDER,
      1.0f,
      VK_FALSE,
      VK_FALSE
  );

  VkPipelineMultisampleStateCreateInfo msCI = { };
  msCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  msCI.sampleShadingEnable = VK_FALSE;
  msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  msCI.minSampleShading = 1.0f;
  msCI.pSampleMask = nullptr;
  msCI.alphaToOneEnable = VK_FALSE;
  msCI.alphaToCoverageEnable = VK_FALSE;
  
  VkPipelineDepthStencilStateCreateInfo depthStencilCI = { };
  depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilCI.depthTestEnable = VK_TRUE;
  depthStencilCI.depthWriteEnable = VK_TRUE;
  depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilCI.depthBoundsTestEnable = VK_FALSE;
  depthStencilCI.minDepthBounds = 0.0f;
  depthStencilCI.maxDepthBounds = 1.0f;
  depthStencilCI.stencilTestEnable = VK_FALSE;
  depthStencilCI.back = { };
  depthStencilCI.front = { };

  std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachments;
  colorBlendAttachments[0] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[1] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[2] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[3] = CreateColorBlendAttachmentState(
    VK_FALSE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  VkPipelineColorBlendStateCreateInfo colorBlendCI = CreateBlendStateInfo(
    static_cast<u32>(colorBlendAttachments.size()),
    colorBlendAttachments.data(),
    VK_FALSE,
    VK_LOGIC_OP_COPY
  );

  VkDynamicState dynamicStates[1] = {
    VK_DYNAMIC_STATE_VIEWPORT
  };  

  VkPipelineDynamicStateCreateInfo dynamicCI = { };
  dynamicCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicCI.dynamicStateCount = 1;
  dynamicCI.pDynamicStates = dynamicStates;
  
  VkVertexInputBindingDescription vertBindingDesc = SkinnedVertexDescription::GetBindingDescription();
  auto pbrAttributes = SkinnedVertexDescription::GetVertexAttributes();

  VkPipelineVertexInputStateCreateInfo vertexCI = { };
  vertexCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexCI.vertexBindingDescriptionCount = 1;
  vertexCI.pVertexBindingDescriptions = &vertBindingDesc;
  vertexCI.vertexAttributeDescriptionCount = static_cast<u32>(pbrAttributes.size());
  vertexCI.pVertexAttributeDescriptions = pbrAttributes.data();

  VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = {};
  GraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  GraphicsPipelineInfo.pColorBlendState = &colorBlendCI;
  GraphicsPipelineInfo.pDepthStencilState = &depthStencilCI;
  GraphicsPipelineInfo.pInputAssemblyState = &assemblyCI;
  GraphicsPipelineInfo.pRasterizationState = &rasterizerCI;
  GraphicsPipelineInfo.pMultisampleState = &msCI;
  GraphicsPipelineInfo.pVertexInputState = &vertexCI;
  GraphicsPipelineInfo.pViewportState = &viewportCI;
  GraphicsPipelineInfo.pTessellationState = nullptr;
  GraphicsPipelineInfo.pDynamicState = &dynamicCI;
  GraphicsPipelineInfo.subpass = 0;
  GraphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  // Create the pipeline for the graphics pipeline.
  if (m_pLights && m_pLights->m_pFrameBuffer) {
    colorBlendCI.attachmentCount = 0;
    VkRect2D shadowScissor;
    shadowScissor.extent = { m_pLights->m_pFrameBuffer->Width(), m_pLights->m_pFrameBuffer->Height() };
    shadowScissor.offset = { 0, 0 };
    viewportCI.pScissors = &shadowScissor;
    GraphicsPipelineInfo.renderPass = m_pLights->m_pRenderPass->Handle();
    RendererPass::SetUpDirectionalShadowPass(RHI(), GraphicsPipelineInfo);
    GraphicsPipelineInfo.renderPass = nullptr;
    viewportCI.pScissors = &scissor;
    colorBlendCI.attachmentCount = static_cast<u32>(static_cast<u32>(colorBlendAttachments.size()));
  } else {
    R_DEBUG(rVerbose, "No framebuffer initialized in light data. Skipping shadow map pass...\n");
  }
    
  RendererPass::SetUpForwardPhysicallyBasedPass(RHI(), GraphicsPipelineInfo);

  RendererPass::SetUpGBufferPass(RHI(), GraphicsPipelineInfo);
  RendererPass::SetUpSkyboxPass(RHI(), GraphicsPipelineInfo);

  // Set to quad rendering format.
  colorBlendCI.logicOpEnable = VK_FALSE;
  depthStencilCI.depthTestEnable = VK_FALSE;
  depthStencilCI.depthWriteEnable = VK_FALSE;
  depthStencilCI.stencilTestEnable = VK_FALSE;
  colorBlendCI.attachmentCount = 1; 
  rasterizerCI.cullMode = VK_CULL_MODE_NONE;
  rasterizerCI.polygonMode = VK_POLYGON_MODE_FILL;

  auto finalAttribs = QuadVertexDescription::GetVertexAttributes();
  vertBindingDesc = QuadVertexDescription::GetBindingDescription();
  vertexCI.vertexAttributeDescriptionCount = static_cast<u32>(finalAttribs.size());
  vertexCI.pVertexAttributeDescriptions = finalAttribs.data();

  colorBlendAttachments[0].blendEnable = VK_FALSE;
  colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

  RendererPass::SetUpDeferredPhysicallyBasedPass(RHI(), GraphicsPipelineInfo);
  RendererPass::SetUpDownScalePass(RHI(), GraphicsPipelineInfo);
  RendererPass::SetUpHDRGammaPass(RHI(), GraphicsPipelineInfo);

  if (m_AntiAliasing) {
    RendererPass::SetUpAAPass(RHI(), GraphicsPipelineInfo, m_currentGraphicsConfigs._AA);
  }

  colorBlendAttachments[0].blendEnable = VK_FALSE;
  RendererPass::SetUpFinalPass(RHI(), GraphicsPipelineInfo);
}


void Renderer::CleanUpGraphicsPipelines()
{
  GraphicsPipeline* gbuffer_Pipeline = gbuffer_PipelineKey;
  m_pRhi->FreeGraphicsPipeline(gbuffer_Pipeline);

  GraphicsPipeline* pbr_Pipeline = pbr_Pipeline_LR;
  m_pRhi->FreeGraphicsPipeline(pbr_Pipeline);

  m_pRhi->FreeGraphicsPipeline(pbr_Pipeline_NoLR);

  GraphicsPipeline* gbuffer_StaticPipeline = gbuffer_StaticPipelineKey;
  m_pRhi->FreeGraphicsPipeline(gbuffer_StaticPipeline);

  m_pRhi->FreeGraphicsPipeline(pbr_forwardPipeline_LR);
  m_pRhi->FreeGraphicsPipeline(pbr_staticForwardPipeline_LR);
  m_pRhi->FreeGraphicsPipeline(pbr_forwardPipeline_NoLR);
  m_pRhi->FreeGraphicsPipeline(pbr_staticForwardPipeline_NoLR);

  GraphicsPipeline* QuadPipeline = final_PipelineKey;
  m_pRhi->FreeGraphicsPipeline(QuadPipeline);

  m_pRhi->FreeGraphicsPipeline(output_pipelineKey);

  GraphicsPipeline* HdrPipeline = hdr_gamma_pipelineKey;
  m_pRhi->FreeGraphicsPipeline(HdrPipeline);

  GraphicsPipeline* DownscalePipeline2x = DownscaleBlurPipeline2xKey;
  m_pRhi->FreeGraphicsPipeline(DownscalePipeline2x);

  GraphicsPipeline* DownscalePipeline4x = DownscaleBlurPipeline4xKey;
  m_pRhi->FreeGraphicsPipeline(DownscalePipeline4x);

  GraphicsPipeline* DownscalePipeline8x = DownscaleBlurPipeline8xKey;
  m_pRhi->FreeGraphicsPipeline(DownscalePipeline8x);

  GraphicsPipeline* DownscalePipeline16x = DownscaleBlurPipeline16xKey;
  m_pRhi->FreeGraphicsPipeline(DownscalePipeline16x);

  GraphicsPipeline* GlowPipeline = GlowPipelineKey;
  m_pRhi->FreeGraphicsPipeline(GlowPipeline);

  GraphicsPipeline* SkyPipeline = skybox_pipelineKey;
  m_pRhi->FreeGraphicsPipeline(SkyPipeline);

  GraphicsPipeline* ShadowMapPipeline = ShadowMapPipelineKey;
  GraphicsPipeline* DynamicShadowMapPipline = DynamicShadowMapPipelineKey;
  if (ShadowMapPipeline) {
    m_pRhi->FreeGraphicsPipeline(ShadowMapPipeline);
  }
  
  if (DynamicShadowMapPipline) {
    m_pRhi->FreeGraphicsPipeline(DynamicShadowMapPipline);
  }
}


void Renderer::CleanUpFrameBuffers()
{
  FrameBuffer* gbuffer_FrameBuffer = gbuffer_FrameBufferKey;
  m_pRhi->FreeRenderPass(gbuffer_renderPass);
  m_pRhi->FreeFrameBuffer(gbuffer_FrameBuffer);

  FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;
  m_pRhi->FreeRenderPass(pbr_renderPass);
  m_pRhi->FreeFrameBuffer(pbr_FrameBuffer);

  FrameBuffer* hdrFrameBuffer = hdr_gamma_frameBufferKey;
  m_pRhi->FreeRenderPass(hdr_renderPass);
  m_pRhi->FreeFrameBuffer(hdrFrameBuffer);

  FrameBuffer* DownScaleFB2x = FrameBuffer2xHorizKey;
  FrameBuffer* FB2xFinal = FrameBuffer2xFinalKey;
  FrameBuffer* DownScaleFB4x = FrameBuffer4xKey;
  FrameBuffer* FB4xFinal = FrameBuffer4xFinalKey;
  FrameBuffer* DownScaleFB8x = FrameBuffer8xKey;
  FrameBuffer* FB8xFinal = FrameBuffer8xFinalKey;
  FrameBuffer* DownScaleFB16x = FrameBuffer16xKey;
  FrameBuffer* FB16xFinal = FrameBuffer16xFinalKey;
  FrameBuffer* GlowFB = FrameBufferGlowKey;

  m_pRhi->FreeFrameBuffer(DownScaleFB2x);
  m_pRhi->FreeFrameBuffer(DownScaleFB4x);
  m_pRhi->FreeFrameBuffer(DownScaleFB8x);
  m_pRhi->FreeFrameBuffer(DownScaleFB16x);
  m_pRhi->FreeFrameBuffer(FB2xFinal);
  m_pRhi->FreeFrameBuffer(FB4xFinal);
  m_pRhi->FreeFrameBuffer(FB8xFinal);
  m_pRhi->FreeFrameBuffer(FB16xFinal);
  m_pRhi->FreeFrameBuffer(GlowFB);
  
  m_pRhi->FreeRenderPass(final_renderPass);
  m_pRhi->FreeFrameBuffer(final_frameBufferKey);

  m_pRhi->FreeRenderPass(pbr_forwardRenderPass);
}


void Renderer::SetUpRenderTextures(b32 fullSetup)
{
  Texture* renderTarget2xScaled = m_pRhi->CreateTexture();
  Texture* RenderTarget2xFinal = m_pRhi->CreateTexture();
  Texture* renderTarget4xScaled = m_pRhi->CreateTexture();
  Texture* RenderTarget4xFinal = m_pRhi->CreateTexture();
  Texture* renderTarget8xScaled = m_pRhi->CreateTexture();
  Texture* RenderTarget8xFinal = m_pRhi->CreateTexture();
  Texture* RenderTarget16xScaled = m_pRhi->CreateTexture();
  Texture* RenderTarget16xFinal = m_pRhi->CreateTexture();

  Texture* pbr_Final = m_pRhi->CreateTexture();
  Texture* pbr_Bright = m_pRhi->CreateTexture();
  Texture* GlowTarget = m_pRhi->CreateTexture();

  Texture* gbuffer_Albedo = m_pRhi->CreateTexture();
  Texture* gbuffer_Normal = m_pRhi->CreateTexture();
  Texture* gbuffer_roughMetalSpec = m_pRhi->CreateTexture();
  Texture* gbuffer_Emission = m_pRhi->CreateTexture();
  Texture* gbuffer_Depth = m_pRhi->CreateTexture();
  Sampler* gbuffer_Sampler = m_pRhi->CreateSampler();

  Texture* hdr_Texture = m_pRhi->CreateTexture();
  Sampler* hdr_Sampler = m_pRhi->CreateSampler();
  
  Texture* final_renderTexture = m_pRhi->CreateTexture();

  hdr_gamma_samplerKey = hdr_Sampler;
  hdr_gamma_colorAttachKey = hdr_Texture;
  gbuffer_AlbedoAttachKey = gbuffer_Albedo;
  gbuffer_NormalAttachKey = gbuffer_Normal;
  gbuffer_PositionAttachKey = gbuffer_roughMetalSpec;
  gbuffer_EmissionAttachKey = gbuffer_Emission;
  gbuffer_DepthAttachKey = gbuffer_Depth;
  pbr_FinalTextureKey = pbr_Final;
  pbr_BrightTextureKey = pbr_Bright;
  RenderTarget2xHorizKey = renderTarget2xScaled;
  RenderTarget2xFinalKey = RenderTarget2xFinal;
  RenderTarget4xScaledKey = renderTarget4xScaled;
  RenderTarget4xFinalKey = RenderTarget4xFinal;
  RenderTarget8xScaledKey = renderTarget8xScaled;
  RenderTarget8xFinalKey = RenderTarget8xFinal;
  RenderTarget16xScaledKey = RenderTarget16xScaled;
  RenderTarget16xFinalKey = RenderTarget16xFinal;
  RenderTargetGlowKey = GlowTarget;
  final_renderTargetKey = final_renderTexture;
  gbuffer_SamplerKey = gbuffer_Sampler;
  
  VkImageCreateInfo cImageInfo = { };
  VkImageViewCreateInfo cViewInfo = { };
  VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();

  // TODO(): Need to make this more adaptable, as intel chips have trouble with srgb optimal tiling.
  cImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  cImageInfo.imageType = VK_IMAGE_TYPE_2D;
  cImageInfo.format = GBUFFER_ALBEDO_FORMAT;
  cImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  cImageInfo.mipLevels = 1;
  cImageInfo.extent.depth = 1;
  cImageInfo.arrayLayers = 1;
  cImageInfo.extent.width = windowExtent.width;
  cImageInfo.extent.height = windowExtent.height;
  cImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  cImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  cImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

  cViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
  cViewInfo.format = GBUFFER_ALBEDO_FORMAT;
  cViewInfo.image = nullptr; // No need to set the image, texture->Initialize() handles this for us.
  cViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  cViewInfo.subresourceRange = { };
  cViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  cViewInfo.subresourceRange.baseArrayLayer = 0;
  cViewInfo.subresourceRange.baseMipLevel = 0;
  cViewInfo.subresourceRange.layerCount = 1;
  cViewInfo.subresourceRange.levelCount = 1;

  gbuffer_Albedo->Initialize(cImageInfo, cViewInfo);  

  cImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  final_renderTexture->Initialize(cImageInfo, cViewInfo);

  cImageInfo.format = GBUFFER_ADDITIONAL_INFO_FORMAT;
  cViewInfo.format =  GBUFFER_ADDITIONAL_INFO_FORMAT;
  gbuffer_Emission->Initialize(cImageInfo, cViewInfo);

  cImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  pbr_Final->Initialize(cImageInfo, cViewInfo);

  cImageInfo.format = GBUFFER_ROUGH_METAL_FORMAT;
  cViewInfo.format = GBUFFER_ROUGH_METAL_FORMAT;
  gbuffer_roughMetalSpec->Initialize(cImageInfo, cViewInfo);

  // For the normal component in the gbuffer we use 10 bits for r, g, and b channels, and 2 bits for alpha.
  // If this is not supported properly, we can go with less precision.
  VkImageFormatProperties imgFmtProps;
  VkResult result = VulkanRHI::gPhysicalDevice.GetImageFormatProperties(VK_FORMAT_R16G16_UNORM, 
    VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
    0, &imgFmtProps);
  cViewInfo.format = GBUFFER_NORMAL_FORMAT;
  cImageInfo.format = GBUFFER_NORMAL_FORMAT;
  if (result == VK_ERROR_FORMAT_NOT_SUPPORTED) { 
    Log(rWarning) << "Graphics GBuffer R10G10B10A2 format for optimal tiling not supported! Switching to R8G8B8A8 format.\n";
    cViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    cImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  }

  cImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  gbuffer_Normal->Initialize(cImageInfo, cViewInfo);

  // TODO(): Need to replace position render target, as we can take advantage of 
  // depth buffer and clip space fragment coordinates.
  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

  pbr_Bright->Initialize(cImageInfo, cViewInfo);
  GlowTarget->Initialize(cImageInfo, cViewInfo);
  // Initialize downscaled render textures.
  cImageInfo.extent.width = windowExtent.width    >> 1;
  cImageInfo.extent.height = windowExtent.height  >> 1;
  renderTarget2xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget2xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = windowExtent.width    >> 2;
  cImageInfo.extent.height = windowExtent.height  >> 2;
  renderTarget4xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget4xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = windowExtent.width    >> 3;
  cImageInfo.extent.height = windowExtent.height  >> 3;
  renderTarget8xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget8xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = windowExtent.width    >> 4;
  cImageInfo.extent.height = windowExtent.height  >> 4;
  RenderTarget16xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget16xFinal->Initialize(cImageInfo, cViewInfo);

  // Depth attachment texture.
  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.extent.width = windowExtent.width;
  cImageInfo.extent.height = windowExtent.height;
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  hdr_Texture->Initialize(cImageInfo, cViewInfo);

  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.extent.width = windowExtent.width;
  cImageInfo.extent.height = windowExtent.height;
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.usage = m_pRhi->DepthUsageFlags() | VK_IMAGE_USAGE_SAMPLED_BIT;
  cImageInfo.format = m_pRhi->DepthFormat();

  cViewInfo.format = m_pRhi->DepthFormat();
  cViewInfo.subresourceRange.aspectMask = m_pRhi->DepthAspectFlags();

  gbuffer_Depth->Initialize(cImageInfo, cViewInfo);

  VkSamplerCreateInfo samplerCI = { };
  samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCI.magFilter = VK_FILTER_LINEAR;
  samplerCI.minFilter = VK_FILTER_LINEAR;
  samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCI.compareEnable = VK_FALSE;
  samplerCI.mipLodBias = 0.0f;
  samplerCI.maxAnisotropy = 16.0f;
  samplerCI.anisotropyEnable = VK_FALSE;
  samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerCI.maxLod = 1.0f;
  samplerCI.minLod = 0.0f;
  samplerCI.unnormalizedCoordinates = VK_FALSE;

  gbuffer_Sampler->Initialize(samplerCI);
  hdr_Sampler->Initialize(samplerCI);

  if (fullSetup) {
    Sampler* defaultSampler = m_pRhi->CreateSampler();
    defaultSampler->Initialize(samplerCI);
    DefaultSampler2DKey = defaultSampler;

    VkImageCreateInfo dImageInfo = {};
    VkImageViewCreateInfo dViewInfo = {};

    dImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    dImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    dImageInfo.imageType = VK_IMAGE_TYPE_2D;
    dImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    dImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dImageInfo.mipLevels = 1;
    dImageInfo.extent.depth = 1;
    dImageInfo.arrayLayers = 1;
    dImageInfo.extent.width = windowExtent.width;
    dImageInfo.extent.height = windowExtent.height;
    dImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    dImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    dImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    dViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    dViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    dViewInfo.image = nullptr; // No need to set the image, texture handles this for us.
    dViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    dViewInfo.subresourceRange = {};
    dViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    dViewInfo.subresourceRange.baseArrayLayer = 0;
    dViewInfo.subresourceRange.baseMipLevel = 0;
    dViewInfo.subresourceRange.layerCount = 1;
    dViewInfo.subresourceRange.levelCount = 1;

    Texture* defaultTexture = m_pRhi->CreateTexture();

    defaultTexture->Initialize(dImageInfo, dViewInfo);
    DefaultTextureKey = defaultTexture;
  }
}


void Renderer::CleanUpRenderTextures(b32 fullCleanup)
{
  {
    Texture* renderTarget2xScaled = RenderTarget2xHorizKey;
    Texture* RenderTarget2xFinal = RenderTarget2xFinalKey;
    Texture* renderTarget4xScaled = RenderTarget4xScaledKey;
    Texture* RenderTarget4xFinal = RenderTarget4xFinalKey;
    Texture* renderTarget8xScaled = RenderTarget8xScaledKey;
    Texture* RenderTarget8xFinal = RenderTarget8xFinalKey;
    Texture* RenderTarget16xScaled = RenderTarget16xScaledKey;
    Texture* RenderTarget16xFinal = RenderTarget16xFinalKey;
    Texture* GlowTarget = RenderTargetGlowKey;

    m_pRhi->FreeTexture(renderTarget2xScaled);
    m_pRhi->FreeTexture(RenderTarget2xFinal);
    m_pRhi->FreeTexture(renderTarget4xScaled);
    m_pRhi->FreeTexture(RenderTarget4xFinal);
    m_pRhi->FreeTexture(RenderTarget8xFinal);
    m_pRhi->FreeTexture(renderTarget8xScaled);
    m_pRhi->FreeTexture(RenderTarget16xScaled);
    m_pRhi->FreeTexture(RenderTarget16xFinal);
    m_pRhi->FreeTexture(GlowTarget);
  }
  {
    Texture* gbuffer_Albedo = gbuffer_AlbedoAttachKey;
    Texture* gbuffer_Normal = gbuffer_NormalAttachKey;
    Texture* gbuffer_Position = gbuffer_PositionAttachKey;
    Texture* gbuffer_Emission = gbuffer_EmissionAttachKey;
    Texture* gbuffer_Depth = gbuffer_DepthAttachKey;
    Sampler* gbuffer_Sampler = gbuffer_SamplerKey;

    Texture* pbr_Bright = pbr_BrightTextureKey;
    Texture* pbr_Final = pbr_FinalTextureKey;
  
    Texture* final_renderTarget = final_renderTargetKey;    

    Texture* hdr_Texture = hdr_gamma_colorAttachKey;
    Sampler* hdr_Sampler = hdr_gamma_samplerKey;
  
    m_pRhi->FreeTexture(hdr_Texture);
    m_pRhi->FreeSampler(hdr_Sampler);

    m_pRhi->FreeTexture(gbuffer_Albedo);
    m_pRhi->FreeTexture(gbuffer_Normal);
    m_pRhi->FreeTexture(gbuffer_Position);
    m_pRhi->FreeTexture(gbuffer_Emission);
    m_pRhi->FreeTexture(gbuffer_Depth);
    m_pRhi->FreeSampler(gbuffer_Sampler);
    m_pRhi->FreeTexture(pbr_Bright);
    m_pRhi->FreeTexture(pbr_Final);
    m_pRhi->FreeTexture(final_renderTarget);
  }
  if (fullCleanup) {
    Texture* defaultTexture = DefaultTextureKey;
    Sampler* defaultSampler = DefaultSampler2DKey;

    m_pRhi->FreeTexture(defaultTexture);
    m_pRhi->FreeSampler(defaultSampler);
  }
}


void Renderer::BuildPbrCmdBuffer() 
{
  GraphicsPipeline* pPipeline = nullptr;
  pPipeline = pbr_Pipeline_NoLR;
  if (m_currentGraphicsConfigs._EnableLocalReflections) {
    pPipeline = pbr_Pipeline_LR;
  }
  CommandBuffer* cmdBuffer = m_Pbr._CmdBuffer;
  if (cmdBuffer) {
    cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  }

  FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;

  VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
  VkViewport viewport = {};
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  // Start the next pass. The PBR Pass.
  std::array<VkClearValue, 3> clearValuesPBR;
  clearValuesPBR[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValuesPBR[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValuesPBR[2].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo pbr_RenderPassInfo = {};
  pbr_RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  pbr_RenderPassInfo.framebuffer = pbr_FrameBuffer->Handle();
  pbr_RenderPassInfo.renderPass = pbr_renderPass->Handle();
  pbr_RenderPassInfo.pClearValues = clearValuesPBR.data();
  pbr_RenderPassInfo.clearValueCount = static_cast<u32>(clearValuesPBR.size());
  pbr_RenderPassInfo.renderArea.extent = windowExtent;
  pbr_RenderPassInfo.renderArea.offset = { 0, 0 };

  const u32 dSetCount = 5;
  cmdBuffer->Begin(beginInfo);
    cmdBuffer->BeginRenderPass(pbr_RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    GraphicsPipeline* pbr_Pipeline = pPipeline;
    VkDescriptorSet sets[dSetCount] = {
      m_pGlobal->Set()->Handle(),
      pbr_DescSetKey->Handle(),
      m_pLights->Set()->Handle(),
      m_pLights->ViewSet()->Handle(),
      m_pGlobalIllumination->Handle()
    };
    cmdBuffer->SetViewPorts(0, 1, &viewport);
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_Pipeline->Pipeline());
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_Pipeline->Layout(), 0, dSetCount, sets, 0, nullptr);
    VkBuffer vertexBuffer = m_RenderQuad.Quad()->Handle()->NativeBuffer();
    VkBuffer indexBuffer = m_RenderQuad.Indices()->Handle()->NativeBuffer();
    VkDeviceSize offsets[] = { 0 };
    cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmdBuffer->BindIndexBuffer(indexBuffer, 0, GetNativeIndexType(m_RenderQuad.Indices()->GetSizeType()));
    cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::SetUpOffscreen(b32 fullSetup)
{
  if (fullSetup) {
    m_Offscreen._Semaphore = m_pRhi->CreateVkSemaphore();
    VkSemaphoreCreateInfo semaCI = { };
    semaCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_Offscreen._Semaphore->Initialize(semaCI);

    for (size_t i = 0; i < m_Offscreen._CmdBuffers.size(); ++i) {
      m_Offscreen._CmdBuffers[i] = m_pRhi->CreateCommandBuffer();
      m_Offscreen._CmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
      m_Offscreen._ShadowCmdBuffers[i] = m_pRhi->CreateCommandBuffer();
      m_Offscreen._ShadowCmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(1), VK_COMMAND_BUFFER_LEVEL_PRIMARY);  
    }
  }
}


void Renderer::CleanUpOffscreen(b32 fullCleanup)
{
  if (fullCleanup) {
    m_pRhi->FreeVkSemaphore(m_Offscreen._Semaphore);
    for (size_t i = 0; i < m_Offscreen._CmdBuffers.size(); ++i) {
      m_pRhi->FreeCommandBuffer(m_Offscreen._CmdBuffers[i]);
      m_pRhi->FreeCommandBuffer(m_Offscreen._ShadowCmdBuffers[i]);
    }
  }
}


void Renderer::SetUpDownscale(b32 FullSetUp)
{
  if (FullSetUp) {
  }

  DescriptorSetLayout* Layout = DownscaleBlurLayoutKey;
  DescriptorSetLayout* GlowLayout = GlowDescriptorSetLayoutKey;
  DescriptorSet* DBDS2x = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS2xFinal = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS4x = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS4xFinal = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS8x = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS8xFinal = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS16x = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS16xFinal = m_pRhi->CreateDescriptorSet();
  DescriptorSet* GlowDS = m_pRhi->CreateDescriptorSet();
  DownscaleBlurDescriptorSet2x = DBDS2x;
  DownscaleBlurDescriptorSet4x = DBDS4x;
  DownscaleBlurDescriptorSet8x = DBDS8x;
  DownscaleBlurDescriptorSet16x = DBDS16x;
  DownscaleBlurDescriptorSet2xFinalKey = DBDS2xFinal;
  DownscaleBlurDescriptorSet4xFinalKey = DBDS4xFinal;
  DownscaleBlurDescriptorSet8xFinalKey = DBDS8xFinal;
  DownscaleBlurDescriptorSet16xFinalKey = DBDS16xFinal;
  GlowDescriptorSetKey = GlowDS;

  DBDS2x->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS4x->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS8x->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS16x->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS2xFinal->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS4xFinal->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS8xFinal->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS16xFinal->Allocate(m_pRhi->DescriptorPool(), Layout);
  GlowDS->Allocate(m_pRhi->DescriptorPool(), GlowLayout);

  Texture* RTBright = pbr_BrightTextureKey;
  Texture* Color2x = RenderTarget2xHorizKey;
  Texture* Color2xFinal = RenderTarget2xFinalKey;
  Texture* Color4x = RenderTarget4xScaledKey;
  Texture* Color4xFinal = RenderTarget4xFinalKey;
  Texture* Color8x = RenderTarget8xScaledKey;
  Texture* Color8xFinal = RenderTarget8xFinalKey;
  Texture* Color16x = RenderTarget16xScaledKey;
  Texture* Color16xFinal = RenderTarget16xFinalKey;
  Sampler* gbuffer_Sampler = gbuffer_SamplerKey;
  Sampler* DownscaleSampler = ScaledSamplerKey;

  VkDescriptorImageInfo Img = { };
  Img.sampler = gbuffer_Sampler->Handle();
  Img.imageView = RTBright->View();
  Img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  
  VkWriteDescriptorSet WriteSet = { };
  WriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  WriteSet.descriptorCount = 1;
  WriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  WriteSet.dstArrayElement = 0;
  WriteSet.dstBinding = 0;
  WriteSet.pBufferInfo = nullptr;
  WriteSet.pImageInfo = &Img;
  WriteSet.pNext = nullptr;
  WriteSet.pTexelBufferView = nullptr;
  
  DBDS2x->Update(1, &WriteSet);
  Img.imageView = Color2x->View();
  Img.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  DBDS2xFinal->Update(1, &WriteSet);
  Img.imageView = Color2xFinal->View();
  DBDS4x->Update(1, &WriteSet);
  Img.imageView = Color4x->View();
  DBDS4xFinal->Update(1, &WriteSet);
  Img.imageView = Color4xFinal->View();
  DBDS8x->Update(1, &WriteSet);
  Img.imageView = Color8x->View();
  DBDS8xFinal->Update(1, &WriteSet);
  Img.imageView = Color8xFinal->View();
  DBDS16x->Update(1, &WriteSet);
  Img.imageView = Color16x->View();
  DBDS16xFinal->Update(1, &WriteSet);

  Img.imageView = Color2xFinal->View();

  VkDescriptorImageInfo Img1 = { };
  Img1.sampler = gbuffer_Sampler->Handle();
  Img1.imageView = Color4xFinal->View();
  Img1.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkDescriptorImageInfo Img2 = { };
  Img2.sampler = gbuffer_Sampler->Handle();
  Img2.imageView = Color8xFinal->View();
  Img2.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkDescriptorImageInfo Img3 = { };
  Img3.sampler = gbuffer_Sampler->Handle();
  Img3.imageView = Color16xFinal->View();
  Img3.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Glow buffer.
  std::array<VkWriteDescriptorSet, 4> GlowWrites;
  GlowWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  GlowWrites[0].descriptorCount = 1;
  GlowWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  GlowWrites[0].dstArrayElement = 0;
  GlowWrites[0].dstBinding = 0;
  GlowWrites[0].pBufferInfo = nullptr;
  GlowWrites[0].pImageInfo = &Img;
  GlowWrites[0].pNext = nullptr;
  GlowWrites[0].pTexelBufferView = nullptr;

  GlowWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  GlowWrites[1].descriptorCount = 1;
  GlowWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  GlowWrites[1].dstArrayElement = 0;
  GlowWrites[1].dstBinding = 1;
  GlowWrites[1].pBufferInfo = nullptr;
  GlowWrites[1].pImageInfo = &Img1;
  GlowWrites[1].pNext = nullptr;
  GlowWrites[1].pTexelBufferView = nullptr;

  GlowWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  GlowWrites[2].descriptorCount = 1;
  GlowWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  GlowWrites[2].dstArrayElement = 0;
  GlowWrites[2].dstBinding = 2;
  GlowWrites[2].pBufferInfo = nullptr;
  GlowWrites[2].pImageInfo = &Img2;
  GlowWrites[2].pNext = nullptr;
  GlowWrites[2].pTexelBufferView = nullptr;

  GlowWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  GlowWrites[3].descriptorCount = 1;
  GlowWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  GlowWrites[3].dstArrayElement = 0;
  GlowWrites[3].dstBinding = 3;
  GlowWrites[3].pBufferInfo = nullptr;
  GlowWrites[3].pImageInfo = &Img3;
  GlowWrites[3].pNext = nullptr;
  GlowWrites[3].pTexelBufferView = nullptr;

  GlowDS->Update(static_cast<u32>(GlowWrites.size()), GlowWrites.data());
}


void Renderer::CleanUpDownscale(b32 FullCleanUp)
{

  DescriptorSet* DBDS2x = DownscaleBlurDescriptorSet2x;
  m_pRhi->FreeDescriptorSet(DBDS2x);
  DescriptorSet* DBDS4x = DownscaleBlurDescriptorSet4x;
  m_pRhi->FreeDescriptorSet(DBDS4x);
  DescriptorSet* DBDS8x = DownscaleBlurDescriptorSet8x;
  m_pRhi->FreeDescriptorSet(DBDS8x);
  DescriptorSet* DBDS16x = DownscaleBlurDescriptorSet16x;
  m_pRhi->FreeDescriptorSet(DBDS16x);
  DescriptorSet* DBDS2xFinal = DownscaleBlurDescriptorSet2xFinalKey;
  m_pRhi->FreeDescriptorSet(DBDS2xFinal);
  DescriptorSet* DBDS4xFinal = DownscaleBlurDescriptorSet4xFinalKey;
  m_pRhi->FreeDescriptorSet(DBDS4xFinal);
  DescriptorSet* DBDS8xFinal = DownscaleBlurDescriptorSet8xFinalKey;
  m_pRhi->FreeDescriptorSet(DBDS8xFinal);
  DescriptorSet* DBDS16xFinal = DownscaleBlurDescriptorSet16xFinalKey;
  m_pRhi->FreeDescriptorSet(DBDS16xFinal);
  DescriptorSet* GlowDS = GlowDescriptorSetKey;
  m_pRhi->FreeDescriptorSet(GlowDS);
}


void Renderer::SetUpHDR(b32 fullSetUp)
{
  if (fullSetUp) {
    m_HDR._CmdBuffer = m_pRhi->CreateCommandBuffer();
    m_HDR._CmdBuffer->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    m_HDR._Semaphore = m_pRhi->CreateVkSemaphore();
    VkSemaphoreCreateInfo semaCi = { };
    semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_HDR._Semaphore->Initialize(semaCi);
  }

  DescriptorSet* hdrSet = m_pRhi->CreateDescriptorSet();
  hdr_gamma_descSetKey = hdrSet;
  std::array<VkWriteDescriptorSet, 3> hdrWrites;
  VkDescriptorBufferInfo hdrBufferInfo = {};
  hdrBufferInfo.offset = 0;
  hdrBufferInfo.range = sizeof(GlobalBuffer);
  hdrBufferInfo.buffer = m_pGlobal->Handle()->NativeBuffer();

  VkDescriptorImageInfo pbrImageInfo = { };
  pbrImageInfo.sampler = gbuffer_SamplerKey->Handle();
  pbrImageInfo.imageView = pbr_FinalTextureKey->View();
  pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // TODO(): We don't have our bloom pipeline and texture yet, we will sub it with this instead!
  VkDescriptorImageInfo bloomImageInfo = { };
  bloomImageInfo.sampler = gbuffer_SamplerKey->Handle();
  bloomImageInfo.imageView = RenderTargetGlowKey->View();
  bloomImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  hdrWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  hdrWrites[0].descriptorCount = 1;
  hdrWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  hdrWrites[0].dstArrayElement = 0;
  hdrWrites[0].dstBinding = 0;
  hdrWrites[0].pImageInfo = &pbrImageInfo; 
  hdrWrites[0].pBufferInfo = nullptr;
  hdrWrites[0].pTexelBufferView = nullptr;
  hdrWrites[0].pNext = nullptr;

  hdrWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  hdrWrites[1].descriptorCount = 1;
  hdrWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  hdrWrites[1].dstArrayElement = 0;
  hdrWrites[1].dstBinding = 1;
  hdrWrites[1].pImageInfo = &bloomImageInfo; //
  hdrWrites[1].pBufferInfo = nullptr;
  hdrWrites[1].pTexelBufferView = nullptr;
  hdrWrites[1].pNext = nullptr;

  hdrWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  hdrWrites[2].descriptorCount = 1;
  hdrWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  hdrWrites[2].dstArrayElement = 0;
  hdrWrites[2].dstBinding = 2;
  hdrWrites[2].pImageInfo = nullptr; //
  hdrWrites[2].pBufferInfo = &hdrBufferInfo;
  hdrWrites[2].pTexelBufferView = nullptr;
  hdrWrites[2].pNext = nullptr;

  // Allocate and update the hdr buffer.
  hdrSet->Allocate(m_pRhi->DescriptorPool(), hdr_gamma_descSetLayoutKey);
  hdrSet->Update(static_cast<u32>(hdrWrites.size()), hdrWrites.data());
}


void Renderer::CleanUpHDR(b32 fullCleanUp)
{
  if (fullCleanUp) {
    m_pRhi->FreeVkSemaphore(m_HDR._Semaphore);

    m_pRhi->FreeCommandBuffer(m_HDR._CmdBuffer);
    m_HDR._CmdBuffer = nullptr;

    m_HDR._Semaphore = nullptr;
  }

  DescriptorSet* hdrSet = hdr_gamma_descSetKey;
  m_pRhi->FreeDescriptorSet(hdrSet);
}


void Renderer::Build()
{
  m_pRhi->WaitAllGraphicsQueues();

  BuildOffScreenBuffer(CurrentCmdBufferIdx());
  BuildHDRCmdBuffer();
  BuildShadowCmdBuffer(CurrentCmdBufferIdx());
  BuildPbrCmdBuffer();
  BuildSkyboxCmdBuffer();
  BuildFinalCmdBuffer();
  m_pRhi->RebuildCommandBuffers(m_pRhi->CurrentSwapchainCmdBufferSet());
}


void Renderer::SetUpSkybox()
{
  DescriptorSet* skyboxSet = m_pRhi->CreateDescriptorSet();
  skybox_descriptorSetKey = skyboxSet;
  DescriptorSetLayout* layout = skybox_setLayoutKey;

  Texture* cubemap = m_pSky->GetCubeMap();
  VkDescriptorImageInfo image = { };
  image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image.imageView = cubemap->View();
  image.sampler = m_pSky->GetSampler()->Handle();

  VkWriteDescriptorSet write = { };
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstArrayElement = 0;
  write.dstBinding = 0;
  write.pImageInfo = &image;

  skyboxSet->Allocate(m_pRhi->DescriptorPool(), layout);
  skyboxSet->Update(1, &write);

  // Create skybox Commandbuffer.
  m_pSkyboxCmdBuffer = m_pRhi->CreateCommandBuffer();
  m_pSkyboxCmdBuffer->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  m_SkyboxFinished = m_pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo sema = { };
  sema.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_SkyboxFinished->Initialize(sema);
}


void Renderer::CleanUpSkybox()
{
  DescriptorSet* skyboxSet = skybox_descriptorSetKey;
  m_pRhi->FreeDescriptorSet(skyboxSet);

  // Cleanup commandbuffer for skybox.
  m_pRhi->FreeCommandBuffer(m_pSkyboxCmdBuffer);
  m_pSkyboxCmdBuffer = nullptr;

  m_pRhi->FreeVkSemaphore(m_SkyboxFinished);
  m_SkyboxFinished = nullptr;
}


void Renderer::BuildSkyboxCmdBuffer()
{
  if (m_pSkyboxCmdBuffer) {
    m_pSkyboxCmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  }

  VkCommandBufferBeginInfo beginInfo = { };
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  
  CommandBuffer* buf = m_pSkyboxCmdBuffer;
  FrameBuffer* skyFrameBuffer = pbr_FrameBufferKey;
  GraphicsPipeline* skyPipeline = skybox_pipelineKey;
  DescriptorSet* global = m_pGlobal->Set();
  DescriptorSet* skybox = skybox_descriptorSetKey;

  VkDescriptorSet descriptorSets[] = {
    global->Handle(),
    skybox->Handle()
  };  

  buf->Begin(beginInfo);
    std::array<VkClearValue, 3> clearValues;
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[2].depthStencil = { 1.0f, 0 };

    VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
    VkViewport viewport = {};
    viewport.height = (r32)windowExtent.height;
    viewport.width = (r32)windowExtent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.y = 0.0f;
    viewport.x = 0.0f;

    VkRenderPassBeginInfo renderBegin = { };
    renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBegin.framebuffer = skyFrameBuffer->Handle();
    renderBegin.renderPass = m_pSky->GetSkyboxRenderPass()->Handle();
    renderBegin.clearValueCount = static_cast<u32>(clearValues.size());
    renderBegin.pClearValues = clearValues.data();
    renderBegin.renderArea.offset = { 0, 0 };
    renderBegin.renderArea.extent = windowExtent;
    
    // Start the renderpass.
    buf->BeginRenderPass(renderBegin, VK_SUBPASS_CONTENTS_INLINE);
      buf->SetViewPorts(0, 1, &viewport);
      buf->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->Pipeline());
      buf->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->Layout(), 0, 2, descriptorSets, 0, nullptr);
      VertexBuffer* vertexbuffer = m_pSky->GetSkyboxVertexBuffer();
      IndexBuffer* idxBuffer = m_pSky->GetSkyboxIndexBuffer();

      VkDeviceSize offsets[] =  { 0 };
      VkBuffer vert = vertexbuffer->Handle()->NativeBuffer();
      VkBuffer ind = idxBuffer->Handle()->NativeBuffer();
      buf->BindVertexBuffers(0 , 1, &vert, offsets);  
      buf->BindIndexBuffer(ind, 0, GetNativeIndexType(idxBuffer->GetSizeType()));
      buf->DrawIndexed(idxBuffer->IndexCount(), 1, 0, 0, 0);
    buf->EndRenderPass();
  buf->End();
}


void Renderer::BuildOffScreenBuffer(u32 cmdBufferIndex)
{
  if (cmdBufferIndex >= m_Offscreen._CmdBuffers.size()) { 
    R_DEBUG(rError, "Attempted to build offscreen cmd buffer. Index out of bounds!\n");
    return; 
  }

  if (!m_pLights || !m_pGlobal) {  
    Log(rWarning) << "Can not build commandbuffers without light or global data! One of them is null!";
  } 

  CommandBuffer* cmdBuffer = m_Offscreen._CmdBuffers[cmdBufferIndex];
  FrameBuffer* gbuffer_FrameBuffer = gbuffer_FrameBufferKey;
  GraphicsPipeline* gbuffer_Pipeline = gbuffer_PipelineKey;
  GraphicsPipeline* gbuffer_StaticPipeline = gbuffer_StaticPipelineKey;

  if (cmdBuffer && !cmdBuffer->Recording()) {
    cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  }

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  std::array<VkClearValue, 5> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[4].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo gbuffer_RenderPassInfo = {};
  VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
  gbuffer_RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  gbuffer_RenderPassInfo.framebuffer = gbuffer_FrameBuffer->Handle();
  gbuffer_RenderPassInfo.renderPass = gbuffer_renderPass->Handle();
  gbuffer_RenderPassInfo.pClearValues = clearValues.data();
  gbuffer_RenderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
  gbuffer_RenderPassInfo.renderArea.extent = windowExtent;
  gbuffer_RenderPassInfo.renderArea.offset = { 0, 0 };

  VkViewport viewport =  { };
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  VkDescriptorSet DescriptorSets[6];
  
  cmdBuffer->Begin(beginInfo);
    cmdBuffer->BeginRenderPass(gbuffer_RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    for (size_t i = 0; i < m_cmdDeferredList.Size(); ++i) {
      MeshRenderCmd& renderCmd = m_cmdDeferredList.Get(i);
      // Need to notify that this render command does not have a render object.
      if (!renderCmd._pMeshDesc) continue;
      if (!(renderCmd._config & CMD_RENDERABLE_BIT) ||
          (renderCmd._config & (CMD_TRANSPARENT_BIT | CMD_TRANSLUCENT_BIT))) continue;
      if (!renderCmd._pMeshData) {
        R_DEBUG(rWarning, "Null data in render mesh!, skipping...\n");
        continue;
      }

      MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
      b8 Skinned = pMeshDesc->Skinned();
      GraphicsPipeline* Pipe = Skinned ? gbuffer_Pipeline : gbuffer_StaticPipeline;

      // Set up the render mesh
      MeshData* data = renderCmd._pMeshData;
      // TODO(): Do culling if needed here.
      VertexBuffer* vertexBuffer = data->VertexData();
      IndexBuffer* indexBuffer = data->IndexData();
      VkBuffer vb = vertexBuffer->Handle()->NativeBuffer();

      VkDeviceSize offsets[] = { 0 };
      cmdBuffer->BindVertexBuffers(0, 1, &vb, offsets);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipe->Pipeline());
      cmdBuffer->SetViewPorts(0, 1, &viewport);

      DescriptorSets[0] = m_pGlobal->Set()->Handle();
      DescriptorSets[1] = pMeshDesc->CurrMeshSet()->Handle();
      DescriptorSets[3] = (Skinned ? pMeshDesc->CurrJointSet()->Handle() : nullptr);

      if (indexBuffer) {
        VkBuffer ib = indexBuffer->Handle()->NativeBuffer();
        cmdBuffer->BindIndexBuffer(ib, 0, GetNativeIndexType(indexBuffer->GetSizeType()));
      }

      for (size_t i = 0; i < renderCmd._primitiveCount; ++i) {
        Primitive& primitive = renderCmd._pPrimitives[i];
        MaterialDescriptor* pMatDesc = primitive._pMat;
        DescriptorSets[2] = pMatDesc->CurrMaterialSet()->Handle();
        // Bind materials.
        cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, 
          Pipe->Layout(), 0, (Skinned ? 4 : 3), DescriptorSets, 0, nullptr);
        if (indexBuffer) {
          cmdBuffer->DrawIndexed(primitive._indexCount, renderCmd._instances, primitive._firstIndex, 0, 0);
        } else {
          cmdBuffer->Draw(vertexBuffer->VertexCount(), renderCmd._instances, 0, 0);
        }
      }
    }
  cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::BuildFinalCmdBuffer()
{
  if (!m_pFinalCommandBuffer) return;

  CommandBuffer* cmdBuffer = m_pFinalCommandBuffer;
  cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

  VkCommandBufferBeginInfo cmdBi = {};
  cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
  // Do stuff with the buffer.
  VkViewport viewport = {};
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.x = 0.0f;
  viewport.y = 0.0f;

  GraphicsPipeline* finalPipeline = final_PipelineKey;
  DescriptorSet* finalSet = final_DescSetKey;
  FrameBuffer* finalFrameBuffer = final_frameBufferKey;

  VkClearValue clearVal = {};
  clearVal.color = { 0.0f, 0.0f, 0.0f, 1.0f };

  VkRenderPassBeginInfo renderpassInfo = { };
  renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderpassInfo.framebuffer = finalFrameBuffer->Handle();
  renderpassInfo.clearValueCount = 1;
  renderpassInfo.pClearValues = &clearVal;
  renderpassInfo.renderPass = final_renderPass->Handle();
  renderpassInfo.renderArea.extent = windowExtent;
  renderpassInfo.renderArea.offset = { 0, 0 };

  cmdBuffer->Begin(cmdBi);
    cmdBuffer->BeginRenderPass(renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Pipeline());
      VkDescriptorSet finalDescriptorSets[] = { finalSet->Handle() };

      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Layout(), 0, 1, finalDescriptorSets, 0, nullptr);
      VkBuffer vertexBuffer = m_RenderQuad.Quad()->Handle()->NativeBuffer();
      VkBuffer indexBuffer = m_RenderQuad.Indices()->Handle()->NativeBuffer();
      VkDeviceSize offsets[] = { 0 };

      cmdBuffer->BindIndexBuffer(indexBuffer, 0, GetNativeIndexType(m_RenderQuad.Indices()->GetSizeType()));
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);

      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::BuildHDRCmdBuffer()
{
  CommandBuffer* cmdBuffer = m_HDR._CmdBuffer;
  if (!cmdBuffer) return;


  VkIndexType indexType = GetNativeIndexType(m_RenderQuad.Indices()->GetSizeType());
  VkBuffer vertexBuffer = m_RenderQuad.Quad()->Handle()->NativeBuffer();
  VkBuffer indexBuffer = m_RenderQuad.Indices()->Handle()->NativeBuffer();
  VkDeviceSize offsets[] = { 0 };

  GraphicsPipeline* hdrPipeline = hdr_gamma_pipelineKey;
  GraphicsPipeline* Downscale2x = DownscaleBlurPipeline2xKey;
  GraphicsPipeline* Downscale4x = DownscaleBlurPipeline4xKey;
  GraphicsPipeline* Downscale8x = DownscaleBlurPipeline8xKey;
  GraphicsPipeline* Downscale16x = DownscaleBlurPipeline16xKey;
  GraphicsPipeline* GlowPipeline = GlowPipelineKey;
  FrameBuffer* hdrFrameBuffer = hdr_gamma_frameBufferKey;
  FrameBuffer* DownscaleFrameBuffer2x = FrameBuffer2xHorizKey;
  FrameBuffer* FB2xFinal = FrameBuffer2xFinalKey;
  FrameBuffer* DownscaleFrameBuffer4x = FrameBuffer4xKey;
  FrameBuffer* FB4xFinal = FrameBuffer4xFinalKey;
  FrameBuffer* DownscaleFrameBuffer8x = FrameBuffer8xKey;
  FrameBuffer* FB8xFinal = FrameBuffer8xFinalKey;
  FrameBuffer* DownscaleFrameBuffer16x = FrameBuffer16xKey;
  FrameBuffer* FB16xFinal = FrameBuffer16xFinalKey;
  FrameBuffer* GlowFrameBuffer = FrameBufferGlowKey;
  DescriptorSet* hdrSet = hdr_gamma_descSetKey;
  DescriptorSet* DownscaleSet2x = DownscaleBlurDescriptorSet2x;
  DescriptorSet* DownscaleSet4x = DownscaleBlurDescriptorSet4x;
  DescriptorSet* DownscaleSet8x = DownscaleBlurDescriptorSet8x;
  DescriptorSet* DownscaleSet16x = DownscaleBlurDescriptorSet16x;
  DescriptorSet* DownscaleSet2xFinal = DownscaleBlurDescriptorSet2xFinalKey;
  DescriptorSet* DownscaleSet4xFinal = DownscaleBlurDescriptorSet4xFinalKey;
  DescriptorSet* DownscaleSet8xFinal = DownscaleBlurDescriptorSet8xFinalKey;
  DescriptorSet* DownscaleSet16xFinal = DownscaleBlurDescriptorSet16xFinalKey;
  DescriptorSet* GlowSet = GlowDescriptorSetKey;

  cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  VkCommandBufferBeginInfo cmdBi = { };
  cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  

  VkClearValue clearVal = { };
  clearVal.color = { 0.0f, 0.0f, 0.0f, 1.0f };

  VkRenderPassBeginInfo renderpassInfo = {};
  renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderpassInfo.framebuffer = hdrFrameBuffer->Handle();
  renderpassInfo.clearValueCount = 1;
  renderpassInfo.pClearValues = &clearVal;
  renderpassInfo.renderPass = hdr_renderPass->Handle();
  renderpassInfo.renderArea.extent = m_pRhi->SwapchainObject()->SwapchainExtent();
  renderpassInfo.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass2x =  { };
  DownscalePass2x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass2x.framebuffer = DownscaleFrameBuffer2x->Handle();
  DownscalePass2x.renderPass = DownscaleFrameBuffer2x->RenderPassRef()->Handle();
  DownscalePass2x.clearValueCount = 1;
  DownscalePass2x.pClearValues = &clearVal;
  DownscalePass2x.renderArea.extent = { DownscaleFrameBuffer2x->Width(), DownscaleFrameBuffer2x->Height() };
  DownscalePass2x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass4x = { };
  DownscalePass4x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass4x.framebuffer = DownscaleFrameBuffer4x->Handle();
  DownscalePass4x.renderPass = DownscaleFrameBuffer4x->RenderPassRef()->Handle();
  DownscalePass4x.clearValueCount = 1;
  DownscalePass4x.pClearValues = &clearVal;
  DownscalePass4x.renderArea.extent = { DownscaleFrameBuffer4x->Width(), DownscaleFrameBuffer4x->Height() };
  DownscalePass4x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass8x = {};
  DownscalePass8x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass8x.framebuffer = DownscaleFrameBuffer8x->Handle();
  DownscalePass8x.renderPass = DownscaleFrameBuffer8x->RenderPassRef()->Handle();
  DownscalePass8x.clearValueCount = 1;
  DownscalePass8x.pClearValues = &clearVal;
  DownscalePass8x.renderArea.extent = { DownscaleFrameBuffer8x->Width(), DownscaleFrameBuffer8x->Height() };
  DownscalePass8x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass16x = {};
  DownscalePass16x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass16x.framebuffer = DownscaleFrameBuffer16x->Handle();
  DownscalePass16x.renderPass = DownscaleFrameBuffer16x->RenderPassRef()->Handle();
  DownscalePass16x.clearValueCount = 1;
  DownscalePass16x.pClearValues = &clearVal;
  DownscalePass16x.renderArea.extent = { DownscaleFrameBuffer16x->Width(), DownscaleFrameBuffer16x->Height() };
  DownscalePass16x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo GlowPass = {};
  GlowPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  GlowPass.framebuffer = GlowFrameBuffer->Handle();
  GlowPass.renderPass = GlowFrameBuffer->RenderPassRef()->Handle();
  GlowPass.clearValueCount = 1;
  GlowPass.pClearValues = &clearVal;
  GlowPass.renderArea.extent = { GlowFrameBuffer->Width(), GlowFrameBuffer->Height() };
  GlowPass.renderArea.offset = { 0, 0 };

  VkViewport viewport = {};
  VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  cmdBuffer->Begin(cmdBi);
    // TODO(): Need to allow switching on/off bloom passing.
    m_Downscale._Strength = 1.35f;
    m_Downscale._Scale = 3.3f;
    m_Downscale._Horizontal = true;
    VkDescriptorSet DownscaleSetNative = DownscaleSet2x->Handle();
    viewport.height = (r32)(windowExtent.height >> 1);
    viewport.width =  (r32)(windowExtent.width  >> 1);
    cmdBuffer->BeginRenderPass(DownscalePass2x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(r32), &m_Downscale._Horizontal);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 4, sizeof(r32), &m_Downscale._Strength);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 8, sizeof(r32), &m_Downscale._Scale);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();
    DownscalePass2x.framebuffer = FB2xFinal->Handle();
    DownscalePass2x.renderPass = FB2xFinal->RenderPassRef()->Handle();
    cmdBuffer->BeginRenderPass(DownscalePass2x, VK_SUBPASS_CONTENTS_INLINE);
      m_Downscale._Horizontal = false;
      DownscaleSetNative = DownscaleSet2xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &m_Downscale._Horizontal);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)(windowExtent.height >> 2);
    viewport.width = (r32)(windowExtent.width   >> 2);
    DownscaleSetNative = DownscaleSet4x->Handle();
    i32 _Horizontal = true;
    cmdBuffer->BeginRenderPass(DownscalePass4x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Pipeline());
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    DownscalePass4x.framebuffer = FB4xFinal->Handle();
    DownscalePass4x.renderPass = FB4xFinal->RenderPassRef()->Handle();
    cmdBuffer->BeginRenderPass(DownscalePass4x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet4xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)(windowExtent.height >> 3);
    viewport.width = (r32)(windowExtent.width   >> 3);
    DownscaleSetNative = DownscaleSet8x->Handle();
    _Horizontal = true;
    cmdBuffer->BeginRenderPass(DownscalePass8x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Pipeline());
      cmdBuffer->PushConstants(Downscale8x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    DownscalePass8x.framebuffer = FB8xFinal->Handle();
    DownscalePass8x.renderPass = FB8xFinal->RenderPassRef()->Handle();
    cmdBuffer->BeginRenderPass(DownscalePass8x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet8xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)(windowExtent.height >> 4);
    viewport.width = (r32)(windowExtent.width   >> 4);
    DownscaleSetNative = DownscaleSet16x->Handle();
    _Horizontal = true;
    cmdBuffer->BeginRenderPass(DownscalePass16x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->Pipeline());
      cmdBuffer->PushConstants(Downscale16x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    DownscalePass16x.framebuffer = FB16xFinal->Handle();
    DownscalePass16x.renderPass = FB16xFinal->RenderPassRef()->Handle();
    cmdBuffer->BeginRenderPass(DownscalePass16x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet16xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale16x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)windowExtent.height;
    viewport.width = (r32)windowExtent.width;
    VkDescriptorSet GlowDescriptorNative = GlowSet->Handle();
    cmdBuffer->BeginRenderPass(GlowPass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Layout(), 0, 1, &GlowDescriptorNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    VkDescriptorSet dSets[1];
    dSets[0] = hdrSet->Handle();

    cmdBuffer->BeginRenderPass(renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Layout(), 0, 1, dSets, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::BuildShadowCmdBuffer(u32 cmdBufferIndex)
{
  if (!m_pLights) return;
  if (!m_pLights->m_pFrameBuffer) return;
  CommandBuffer* cmdBuffer = m_Offscreen._ShadowCmdBuffers[cmdBufferIndex];

  if (!cmdBuffer) {
    return;
  }

  m_pRhi->DeviceWaitIdle();
  cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

  GraphicsPipeline* staticPipeline = ShadowMapPipelineKey;
  GraphicsPipeline* dynamicPipeline = DynamicShadowMapPipelineKey;  
  DescriptorSet*    lightViewSet = m_pLights->m_pLightViewDescriptorSet;  

  VkCommandBufferBeginInfo begin = { };
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  // No need to record as it is already recording?
  if (cmdBuffer->Recording()) return;

  VkRenderPassBeginInfo renderPass = { };
  renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPass.framebuffer = m_pLights->m_pFrameBuffer->Handle();
  renderPass.renderPass = m_pLights->m_pFrameBuffer->RenderPassRef()->Handle();
  renderPass.renderArea.extent = { m_pLights->m_pFrameBuffer->Width(), m_pLights->m_pFrameBuffer->Height() };
  renderPass.renderArea.offset = { 0, 0 };
  VkClearValue depthValue = { };
  depthValue.depthStencil = { 1.0f, 0 };
  renderPass.clearValueCount = 1;
  renderPass.pClearValues = &depthValue;

  VkViewport viewport = {};
  viewport.height = (r32)m_pLights->m_pFrameBuffer->Height();
  viewport.width = (r32)m_pLights->m_pFrameBuffer->Width();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  auto render = [&] (MeshRenderCmd& renderCmd) -> void {
    if (!renderCmd._pMeshDesc) return;
    if (!(renderCmd._config & CMD_RENDERABLE_BIT) || !(renderCmd._config & CMD_SHADOWS_BIT)) return;
    if (!renderCmd._pMeshData) return;
    MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
    b8 skinned = pMeshDesc->Skinned();
    VkDescriptorSet descriptorSets[3];
    descriptorSets[0] = pMeshDesc->CurrMeshSet()->Handle();
    descriptorSets[1] = lightViewSet->Handle();
    descriptorSets[2] = skinned ? pMeshDesc->CurrJointSet()->Handle() : VK_NULL_HANDLE;
    GraphicsPipeline* pipeline = skinned ? dynamicPipeline : staticPipeline;
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Pipeline());
    cmdBuffer->SetViewPorts(0, 1, &viewport);
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Layout(), 0, skinned ? 3 : 2, descriptorSets, 0, nullptr);
    MeshData* mesh = renderCmd._pMeshData;
    VertexBuffer* vertex = mesh->VertexData();
    IndexBuffer* index = mesh->IndexData();
    VkBuffer buf = vertex->Handle()->NativeBuffer();
    VkDeviceSize offset[] = { 0 };
    cmdBuffer->BindVertexBuffers(0, 1, &buf, offset);
    if (index) {
      VkBuffer ind = index->Handle()->NativeBuffer();
      cmdBuffer->BindIndexBuffer(ind, 0, GetNativeIndexType(index->GetSizeType()));
    }

    for (size_t i = 0; i < renderCmd._primitiveCount; ++i) {
      Primitive& primitive = renderCmd._pPrimitives[i];
      if (index) {
        cmdBuffer->DrawIndexed(primitive._indexCount, renderCmd._instances, primitive._firstIndex, 0, 0);
      } else {
        cmdBuffer->Draw(primitive._indexCount, renderCmd._instances, primitive._firstIndex, 0);
      }
    }
  };

  // Create the shadow rendering pass.
  cmdBuffer->Begin(begin);
    cmdBuffer->BeginRenderPass(renderPass, VK_SUBPASS_CONTENTS_INLINE);
      for (size_t i = 0; i < m_cmdDeferredList.Size(); ++i) {
        MeshRenderCmd& renderCmd = m_cmdDeferredList[i];
        render(renderCmd);
      }

      for (size_t i = 0; i < m_forwardCmdList.Size(); ++i) {
        MeshRenderCmd& renderCmd = m_forwardCmdList[i];
        render(renderCmd);
      }
    cmdBuffer->EndRenderPass();    
  cmdBuffer->End();
}


void Renderer::SetUpForwardPBR()
{
  m_Forward._CmdBuffer = m_pRhi->CreateCommandBuffer();
  CommandBuffer* cmdB = m_Forward._CmdBuffer;
  cmdB->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  
  m_Forward._Semaphore = m_pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCi = { };
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_Forward._Semaphore->Initialize(semaCi);
}


void Renderer::CleanUpForwardPBR()
{
  m_pRhi->FreeCommandBuffer(m_Forward._CmdBuffer);
  m_pRhi->FreeVkSemaphore(m_Forward._Semaphore);
}


void Renderer::BuildForwardPBRCmdBuffer()
{
  CommandBuffer* cmdBuffer = m_Forward._CmdBuffer;
  cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  
  VkCommandBufferBeginInfo begin = { };
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  
  std::array<VkClearValue, 5> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[4].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo renderPassCi = {};
  VkExtent2D windowExtent = m_pRhi->SwapchainObject()->SwapchainExtent();
  renderPassCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassCi.framebuffer = pbr_FrameBufferKey->Handle();
  renderPassCi.renderPass = pbr_forwardRenderPass->Handle();
  renderPassCi.pClearValues = clearValues.data();
  renderPassCi.clearValueCount = static_cast<u32>(clearValues.size());
  renderPassCi.renderArea.extent = windowExtent;
  renderPassCi.renderArea.offset = { 0, 0 };

  VkViewport viewport = {};
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;
  
  VkDescriptorSet DescriptorSets[7];

  cmdBuffer->Begin(begin);
    cmdBuffer->BeginRenderPass(renderPassCi, VK_SUBPASS_CONTENTS_INLINE);
      for (size_t i = 0; i < m_forwardCmdList.Size(); ++i) {
        MeshRenderCmd& renderCmd = m_forwardCmdList[i];
        if (!(renderCmd._config & CMD_FORWARD_BIT) || !(renderCmd._config & CMD_RENDERABLE_BIT)) continue;
        if (!renderCmd._pMeshData) {
          R_DEBUG(rWarning, "Null data in render mesh!, skipping...\n");
          continue;
        }

        MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
        b8 Skinned = pMeshDesc->Skinned();
        GraphicsPipeline* Pipe = Skinned ? pbr_forwardPipeline_LR : pbr_staticForwardPipeline_LR;
        cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipe->Pipeline());
        cmdBuffer->SetViewPorts(0, 1, &viewport);

        // Set up the render mesh
        MeshData* data = renderCmd._pMeshData;
        // TODO(): Do culling if needed here.

        VertexBuffer* vertexBuffer = data->VertexData();
        IndexBuffer* indexBuffer = data->IndexData();
        VkBuffer vb = vertexBuffer->Handle()->NativeBuffer();
        VkDeviceSize offsets[] = { 0 };
        cmdBuffer->BindVertexBuffers(0, 1, &vb, offsets);

        if (indexBuffer) {
          VkBuffer ib = indexBuffer->Handle()->NativeBuffer();
          cmdBuffer->BindIndexBuffer(ib, 0, GetNativeIndexType(indexBuffer->GetSizeType()));
        }

        DescriptorSets[0] = m_pGlobal->Set()->Handle();
        DescriptorSets[1] = pMeshDesc->CurrMeshSet()->Handle();
        DescriptorSets[3] = m_pLights->Set()->Handle();
        DescriptorSets[4] = m_pLights->ViewSet()->Handle();
        DescriptorSets[5] = m_pGlobalIllumination->Handle(); // Need global illumination data.
        DescriptorSets[6] = (Skinned ? pMeshDesc->CurrJointSet()->Handle() : nullptr);

        // Bind materials.
        for (size_t i = 0; i < renderCmd._primitiveCount; ++i) {
          Primitive& primitive = renderCmd._pPrimitives[i];
          DescriptorSets[2] = primitive._pMat->CurrMaterialSet()->Handle();
          cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
            Pipe->Layout(),
            0,
            (Skinned ? 7 : 6),
            DescriptorSets,
            0,
            nullptr
          );

          if (indexBuffer) {
            cmdBuffer->DrawIndexed(primitive._indexCount, renderCmd._instances, primitive._firstIndex, 0, 0);
          } else {
            cmdBuffer->Draw(primitive._indexCount, renderCmd._instances, primitive._firstIndex, 0);
          }
        }
      }
    cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::SetUpGlobalIlluminationBuffer()
{
  m_pGlobalIllumination = m_pRhi->CreateDescriptorSet();
  DescriptorSetLayout* layout = nullptr;
  std::array<VkWriteDescriptorSet, 4> writeSets;
  u32 count = 2;

  VkDescriptorImageInfo globalIrrMap = { };
  VkDescriptorImageInfo globalEnvMap = { };
  VkDescriptorImageInfo localIrrMaps = { };
  VkDescriptorImageInfo localEnvMaps = { };

  globalIrrMap.sampler = DefaultSampler2DKey->Handle();
  globalEnvMap.sampler = DefaultSampler2DKey->Handle();
  localIrrMaps.sampler = DefaultSampler2DKey->Handle();
  localEnvMaps.sampler = DefaultSampler2DKey->Handle();

  globalIrrMap.imageView = m_pSky->GetCubeMap()->View();
  globalIrrMap.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalEnvMap.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  localIrrMaps.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  localEnvMaps.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalEnvMap.imageView = m_pSky->GetCubeMap()->View();

  // TODO(): These are place holders, we don't have data for these yet!
  // Obtain env and irr maps from scene when building!
  localIrrMaps.imageView = DefaultTextureKey->View();
  localEnvMaps.imageView = DefaultTextureKey->View();

  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[0].dstBinding = 0;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pImageInfo = &globalIrrMap;
  writeSets[0].dstSet = nullptr;
  writeSets[0].pBufferInfo = nullptr;
  writeSets[0].pTexelBufferView = nullptr;
  writeSets[0].pNext = nullptr;

  writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[1].descriptorCount = 1;
  writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[1].dstBinding = 1;
  writeSets[1].dstArrayElement = 0;
  writeSets[1].pImageInfo = &globalEnvMap;
  writeSets[1].dstSet = nullptr;
  writeSets[1].pBufferInfo = nullptr;
  writeSets[1].pTexelBufferView = nullptr;
  writeSets[1].pNext = nullptr;

  writeSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[2].descriptorCount = 1;
  writeSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[2].dstBinding = 2;
  writeSets[2].dstArrayElement = 0;
  writeSets[2].pImageInfo = &localIrrMaps;
  writeSets[2].dstSet = nullptr;
  writeSets[2].pBufferInfo = nullptr;
  writeSets[2].pTexelBufferView = nullptr;
  writeSets[2].pNext = nullptr;

  writeSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[3].descriptorCount = 1;
  writeSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[3].dstBinding = 3;
  writeSets[3].dstArrayElement = 0;
  writeSets[3].pImageInfo = &localEnvMaps;
  writeSets[3].dstSet = nullptr;
  writeSets[3].pBufferInfo = nullptr;
  writeSets[3].pTexelBufferView = nullptr;
  writeSets[3].pNext = nullptr;

  if (m_currentGraphicsConfigs._EnableLocalReflections) {
    layout = globalIllumination_DescLR;
    count = 4;
  } else {
    layout = globalIllumination_DescNoLR;
  }

  m_pGlobalIllumination->Allocate(m_pRhi->DescriptorPool(), layout);
  m_pGlobalIllumination->Update(count, writeSets.data());
}


void Renderer::CleanUpGlobalIlluminationBuffer()
{
  m_pRhi->FreeDescriptorSet(m_pGlobalIllumination);
  m_pGlobalIllumination = nullptr;
}


void Renderer::SetUpPBR()
{
  Sampler* pbr_Sampler = gbuffer_SamplerKey;

  DescriptorSetLayout* pbr_Layout = pbr_DescLayoutKey;
  DescriptorSet* pbr_Set = m_pRhi->CreateDescriptorSet();
  pbr_DescSetKey = pbr_Set;

  VkDescriptorImageInfo albedo = {};
  albedo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  albedo.imageView = gbuffer_AlbedoAttachKey->View();
  albedo.sampler = pbr_Sampler->Handle();

  VkDescriptorImageInfo normal = {};
  normal.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  normal.imageView = gbuffer_NormalAttachKey->View();
  normal.sampler = pbr_Sampler->Handle();

  VkDescriptorImageInfo position = {};
  position.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  position.imageView = gbuffer_PositionAttachKey->View();
  position.sampler = pbr_Sampler->Handle();

  VkDescriptorImageInfo emission = {};
  emission.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  emission.imageView = gbuffer_EmissionAttachKey->View();
  emission.sampler = pbr_Sampler->Handle();

  VkDescriptorImageInfo depth = { };
  depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depth.imageView = gbuffer_DepthAttachKey->View();
  depth.sampler = pbr_Sampler->Handle();

  std::array<VkWriteDescriptorSet, 5> writeInfo;
  writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[0].descriptorCount = 1;
  writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[0].dstBinding = 0;
  writeInfo[0].pImageInfo = &albedo;
  writeInfo[0].pBufferInfo = nullptr;
  writeInfo[0].pTexelBufferView = nullptr;
  writeInfo[0].dstArrayElement = 0;
  writeInfo[0].pNext = nullptr;

  writeInfo[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[1].descriptorCount = 1;
  writeInfo[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[1].dstBinding = 1;
  writeInfo[1].pImageInfo = &normal;
  writeInfo[1].pBufferInfo = nullptr;
  writeInfo[1].pTexelBufferView = nullptr;
  writeInfo[1].dstArrayElement = 0;
  writeInfo[1].pNext = nullptr;

  writeInfo[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[2].descriptorCount = 1;
  writeInfo[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[2].dstBinding = 2;
  writeInfo[2].pImageInfo = &position;
  writeInfo[2].pBufferInfo = nullptr;
  writeInfo[2].pTexelBufferView = nullptr;
  writeInfo[2].dstArrayElement = 0;
  writeInfo[2].pNext = nullptr;

  writeInfo[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[3].descriptorCount = 1;
  writeInfo[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[3].dstBinding = 3;
  writeInfo[3].pImageInfo = &emission;
  writeInfo[3].pBufferInfo = nullptr;
  writeInfo[3].pTexelBufferView = nullptr;
  writeInfo[3].dstArrayElement = 0;
  writeInfo[3].pNext = nullptr;

  writeInfo[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[4].descriptorCount = 1;
  writeInfo[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[4].dstBinding = 4;
  writeInfo[4].pImageInfo = &depth;
  writeInfo[4].pBufferInfo = nullptr;
  writeInfo[4].pTexelBufferView = nullptr;
  writeInfo[4].dstArrayElement = 0;
  writeInfo[4].pNext = nullptr;
  
  pbr_Set->Allocate(m_pRhi->DescriptorPool(), pbr_Layout);
  pbr_Set->Update(static_cast<u32>(writeInfo.size()), writeInfo.data());

  m_Pbr._CmdBuffer = m_pRhi->CreateCommandBuffer();
  m_Pbr._CmdBuffer->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  m_Pbr._Sema = m_pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCi = {};
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_Pbr._Sema->Initialize(semaCi);
}


void Renderer::CleanUpPBR()
{
  DescriptorSet* pbr_Set = pbr_DescSetKey;
  m_pRhi->FreeDescriptorSet(pbr_Set);

  m_pRhi->FreeCommandBuffer(m_Pbr._CmdBuffer);

  m_pRhi->FreeVkSemaphore(m_Pbr._Sema);
}


void Renderer::SetUpFinalOutputs()
{
  Texture* pbr_Final = pbr_FinalTextureKey;
  Texture* hdr_Color = hdr_gamma_colorAttachKey;

  Sampler* hdr_Sampler = hdr_gamma_samplerKey;
  Sampler* pbr_Sampler = gbuffer_SamplerKey;

  DescriptorSetLayout* finalSetLayout = final_DescSetLayoutKey;
  DescriptorSet* offscreenImageDescriptor = m_pRhi->CreateDescriptorSet();
  final_DescSetKey = offscreenImageDescriptor;
  offscreenImageDescriptor->Allocate(m_pRhi->DescriptorPool(), finalSetLayout);
  {
    // Final texture must be either hdr post process texture, or pbr output without hdr.
    VkDescriptorImageInfo renderTextureFinal = {};
    renderTextureFinal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (m_HDR._Enabled) {
      renderTextureFinal.sampler = hdr_Sampler->Handle();
      renderTextureFinal.imageView = hdr_Color->View();
    } else {
      renderTextureFinal.sampler = pbr_Sampler->Handle();
      renderTextureFinal.imageView = pbr_Final->View();
    }

    std::array<VkWriteDescriptorSet, 1> writeInfo;
    writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo[0].descriptorCount = 1;
    writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeInfo[0].dstBinding = 0;
    writeInfo[0].pImageInfo = &renderTextureFinal;
    writeInfo[0].pBufferInfo = nullptr;
    writeInfo[0].pTexelBufferView = nullptr;
    writeInfo[0].dstArrayElement = 0;
    writeInfo[0].pNext = nullptr;

    offscreenImageDescriptor->Update(static_cast<u32>(writeInfo.size()), writeInfo.data());
  }

  // Output info
  {
    output_descSetKey = m_pRhi->CreateDescriptorSet();
    output_descSetKey->Allocate(m_pRhi->DescriptorPool(), final_DescSetLayoutKey);

    // TODO(): Antialiasing will need to be compensated here, similar to final texture, above.
    VkDescriptorImageInfo renderTextureOut = {};
    renderTextureOut.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    renderTextureOut.imageView = final_renderTargetKey->View();
    renderTextureOut.sampler = hdr_gamma_samplerKey->Handle();

    std::array<VkWriteDescriptorSet, 1> writeInfo;
    writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo[0].descriptorCount = 1;
    writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeInfo[0].dstBinding = 0;
    writeInfo[0].pImageInfo = &renderTextureOut;
    writeInfo[0].pBufferInfo = nullptr;
    writeInfo[0].pTexelBufferView = nullptr;
    writeInfo[0].dstArrayElement = 0;
    writeInfo[0].pNext = nullptr;
    output_descSetKey->Update(static_cast<u32>(writeInfo.size()), writeInfo.data());
  }

  m_pFinalCommandBuffer = m_pRhi->CreateCommandBuffer();
  m_pFinalCommandBuffer->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  m_pFinalFinished = m_pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo info = { };
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO; 
  m_pFinalFinished->Initialize(info); 
}


void Renderer::CheckCmdUpdate()
{
  u32 idx = CurrentCmdBufferIdx() + 1;
  if (idx >= m_TotalCmdBuffers) idx = 0;

  std::thread thr1 = std::thread([&]() -> void {
    BuildOffScreenBuffer(idx);
    BuildForwardPBRCmdBuffer();
  });
  
  if (m_pLights->PrimaryShadowEnabled()) {
    BuildShadowCmdBuffer(idx);
  }

  m_pUI->BuildCmdBuffers(m_uiCmdList, m_pGlobal);

#if 0
  if (m_NeedsUpdate) {
   if (m_AsyncBuild) {
      BuildAsync();
    } else {
      Build();
    }
  }
#endif
  thr1.join();
  m_CurrCmdBufferIdx = idx;
}


void Renderer::RenderPrimaryShadows()
{

}


void Renderer::CleanUpFinalOutputs()
{
  DescriptorSet* offscreenDescriptorSet = final_DescSetKey;
  m_pRhi->FreeDescriptorSet(offscreenDescriptorSet);
  m_pRhi->FreeCommandBuffer(m_pFinalCommandBuffer);
  m_pRhi->FreeVkSemaphore(m_pFinalFinished);
}


MeshData* Renderer::CreateMeshData()
{
  MeshData* mesh = new MeshData();
  mesh->mRhi = m_pRhi;
  return mesh;
}


void Renderer::FreeMeshData(MeshData* mesh)
{
  mesh->CleanUp();
  delete mesh;
}


void Renderer::UpdateSceneDescriptors()
{
  // Update global data.
  m_pGlobal->Data()->_EnableAA = m_AntiAliasing;
  Vector4 vec4 = m_pLights->Data()->_PrimaryLight._Direction;
  Vector4 vAirColor = m_pGlobal->Data()->_vAirColor;
  // Left handed coordinate system, need to flip the z.
  Vector3 sunDir = Vector3(vec4.x, vec4.y, -vec4.z);
  Vector3 airColor = Vector3(vAirColor.x, vAirColor.y, vAirColor.z);
  if (m_pGlobal->Data()->_vSunDir != sunDir && sunDir != Vector3() ||
    m_pSky->GetAirColor() != airColor) {
    m_pGlobal->Data()->_vSunDir = sunDir;
    m_pSky->SetAirColor(airColor);
    m_pSky->MarkDirty();
  }

  // Update the global descriptor.
  m_pGlobal->Update();

  // Update lights in scene.
  Vector4 vViewerPos = m_pGlobal->Data()->_CameraPos;
  m_pLights->SetViewerPosition(Vector3(vViewerPos.x, vViewerPos.y, vViewerPos.z));
  m_pLights->Update();

  // Update mesh descriptors in cmd list.
  for (size_t idx = 0; idx < m_cmdDeferredList.Size(); ++idx) {
    MeshRenderCmd& rnd_cmd = m_cmdDeferredList.Get(idx);

    if (rnd_cmd._pMeshDesc) {
      rnd_cmd._pMeshDesc->Update();
    }

    for (size_t i = 0; i < rnd_cmd._primitiveCount; ++i) {
      Primitive& prim = rnd_cmd._pPrimitives[i];
      R_ASSERT(prim._pMat, "No material descriptor added to this primitive. Need to set a material descriptor!");
      prim._pMat->Update();
    }
  }

  for (size_t idx = 0; idx < m_forwardCmdList.Size(); ++idx) {
    MeshRenderCmd& rnd_cmd = m_forwardCmdList.Get(idx);
    if (rnd_cmd._pMeshDesc) {
      rnd_cmd._pMeshDesc->Update();
    }

    for (size_t i = 0; i < rnd_cmd._primitiveCount; ++i) {
      Primitive& prim = rnd_cmd._pPrimitives[i];
      R_ASSERT(prim._pMat, "No material descriptor added to this primitive. Need to set a material descriptor!");
      prim._pMat->Update();
    }
  }
}


void Renderer::RenderOverlay()
{
  m_pUI->Render();
}


void Renderer::UpdateRuntimeConfigs(const GraphicsConfigParams* params)
{
  m_currentGraphicsConfigs = *params;
  // TODO()::
  switch (params->_AA) {
    case AA_None: m_AntiAliasing = false; break;
    case AA_FXAA_2x:
    default:
      m_AntiAliasing = true; break;
    }

    switch (params->_Shadows) {
    case SHADOWS_NONE:
    {
      m_pLights->EnablePrimaryShadow(false);
      m_pGlobal->Data()->_EnableShadows = false;
    } break;
    case SHADOWS_POTATO:
    case SHADOWS_LOW:
    case SHADOWS_MEDIUM:
    case SHADOWS_HIGH:
    case SHADOWS_ULTRA:
    default:
    {
      m_pLights->EnablePrimaryShadow(true);
      m_pGlobal->Data()->_EnableShadows = true;
    } break;
  }
}


void Renderer::UpdateRendererConfigs(const GraphicsConfigParams* params)
{
  m_pRhi->DeviceWaitIdle();

  if (m_pWindow->Width() <= 0 || m_pWindow->Height() <= 0) { 
    m_Minimized = true;
    return;
  } else {
    m_Minimized = false;
  }

  VkPresentModeKHR presentMode = m_pRhi->SwapchainObject()->CurrentPresentMode();
  u32 bufferCount = 2;
  b32 reconstruct = false;

  if (params) {
    switch (params->_Buffering) {
    case SINGLE_BUFFER: { presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; bufferCount = 1; } break;
    case DOUBLE_BUFFER: { presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR; bufferCount = 2; } break;
    case TRIPLE_BUFFER: { presentMode = VK_PRESENT_MODE_MAILBOX_KHR; bufferCount = 3; } break;
    default: presentMode = m_pRhi->SwapchainObject()->CurrentPresentMode(); break;
    }

    if (params->_EnableVsync >= 1) {
      presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    UpdateRuntimeConfigs(params);
  }

  VkExtent2D extent = m_pRhi->SwapchainObject()->SwapchainExtent();
  if (!params || (presentMode != m_pRhi->SwapchainObject()->CurrentPresentMode()) 
    || (bufferCount != m_pRhi->SwapchainObject()->CurrentBufferCount()) || 
    (extent.height  != m_pWindow->Height() || extent.width != m_pWindow->Width())) {
    reconstruct = true;
  }

  if (reconstruct) {
    // Triple buffering atm, we will need to use user params to switch this.
    m_pRhi->ReConfigure(presentMode, m_pWindow->Width(), m_pWindow->Height(), bufferCount);

    m_pUI->CleanUp();

    CleanUpForwardPBR();
    CleanUpPBR();
    CleanUpHDR(false);
    CleanUpDownscale(false);
    CleanUpOffscreen(false);
    CleanUpFinalOutputs();
    CleanUpGraphicsPipelines();
    CleanUpFrameBuffers();
    CleanUpRenderTextures(false);

    SetUpRenderTextures(false);
    SetUpFrameBuffers();
    SetUpGraphicsPipelines();
    SetUpFinalOutputs();
    SetUpOffscreen(false);
    SetUpDownscale(false);
    SetUpHDR(false);
    SetUpPBR();
    SetUpForwardPBR();
    m_pUI->Initialize(m_pRhi);
  }

  CleanUpGlobalIlluminationBuffer();
  SetUpGlobalIlluminationBuffer();

  Build();
}


void Renderer::BuildAsync()
{
  static b8 inProgress = false;
  // TODO(): building the command buffers asyncronously requires us
  // to allocate temp commandbuffers, build them, and then swap them with
  // the previous commandbuffers.
  std::thread async([&] () -> void {
    if (inProgress) { return; }

    inProgress = true;
    u32 idx = m_pRhi->CurrentImageIndex();

    inProgress = false;
  });
}


void Renderer::WaitIdle()
{
  m_pRhi->DeviceWaitIdle();
}


Texture1D* Renderer::CreateTexture1D()
{
  Texture1D* texture = new Texture1D();
  return texture;
}


void Renderer::FreeTexture1D(Texture1D* texture)
{
  delete texture;
}


Texture2D* Renderer::CreateTexture2D()
{
  Texture2D* texture = new Texture2D();
  texture->mRhi = m_pRhi;

  return texture;
}


void Renderer::FreeTexture2D(Texture2D* texture)
{
  texture->CleanUp();

  delete texture;
}


Texture2DArray* Renderer::CreateTexture2DArray()
{
  Texture2DArray* texture = new Texture2DArray();
  return texture;
}


void Renderer::FreeTexture2DArray(Texture2DArray* texture)
{
  // TODO():
}

void Renderer::FreeTextureCube(TextureCube* texture)
{
  // TODO():
}


Texture3D* Renderer::CreateTexture3D()
{
  Texture3D* texture = new Texture3D();
  return texture;
}


MaterialDescriptor* Renderer::CreateMaterialDescriptor()
{
  MaterialDescriptor* descriptor = new MaterialDescriptor();
  descriptor->m_pRhi = RHI();
  return descriptor;
}


void Renderer::FreeMaterialDescriptor(MaterialDescriptor* descriptor)
{
  if (!descriptor) return;
  descriptor->CleanUp();
  delete descriptor;
}


MeshDescriptor* Renderer::CreateStaticMeshDescriptor()
{
  MeshDescriptor* descriptor = new MeshDescriptor();
  descriptor->m_pRhi = RHI();
  return descriptor;
}


SkinnedMeshDescriptor* Renderer::CreateSkinnedMeshDescriptor()
{
  SkinnedMeshDescriptor* descriptor = new SkinnedMeshDescriptor();
  descriptor->m_pRhi = RHI();
  return descriptor;
}


void Renderer::FreeMeshDescriptor(MeshDescriptor* descriptor)
{
  if (!descriptor) return;
  descriptor->CleanUp();
  delete descriptor; 
}


void Renderer::EnableHDR(b32 enable)
{
  if (m_HDR._Enabled != enable) {
    m_HDR._Enabled = enable;
    UpdateRendererConfigs(nullptr);
  }
}


const char* Renderer::GetDeviceName()
{
  return m_pRhi->DeviceName();
}


void Renderer::SortCmdLists()
{
  m_cmdDeferredList.Sort();
  m_forwardCmdList.Sort();
  // TODO(): Also sort forward list too.
}


void Renderer::ClearCmdLists()
{
  // TODO(): Clear forward command list as well.
  m_cmdDeferredList.Clear();
  m_forwardCmdList.Clear();
}


void Renderer::PushMeshRender(MeshRenderCmd& cmd)
{
  if (m_Minimized) return;

  u32 config = cmd._config;
  if ((config & (CMD_TRANSPARENT_BIT | CMD_TRANSLUCENT_BIT | CMD_FORWARD_BIT))) {
    m_forwardCmdList.PushBack(cmd);
  } else {
    m_cmdDeferredList.PushBack(cmd);
  }
}
} // Recluse