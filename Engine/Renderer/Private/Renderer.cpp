// Copyright (c) 2017 Recluse Project. All rights reserved.
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
#include "RenderObject.hpp"
#include "MeshData.hpp"
#include "LightDescriptor.hpp"
#include "GlobalDescriptor.hpp"
#include "StructuredBuffer.hpp"
#include "VertexDescription.hpp"
#include "SkyAtmosphere.hpp"

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


VkFence cpuFence;


Renderer::Renderer()
  : m_pRhi(nullptr)
  , m_pCmdList(nullptr)
  , m_pDeferredCmdList(nullptr)
  , m_Rendering(false)
  , m_Initialized(false)
  , m_pLights(nullptr)
  , m_pGlobal(nullptr)
  , m_NeedsUpdate(false)
  , m_AsyncBuild(false)
  , m_AntiAliasing(false)
  , m_pSky(nullptr)
  , m_pSkyboxCmdBuffer(nullptr)
  , m_SkyboxFinished(nullptr)
  , m_TotalCmdBuffers(2)
  , m_CurrCmdBufferIdx(0)
{
  m_HDR._Enabled = true;
  m_Offscreen._CmdBuffers.resize(m_TotalCmdBuffers);
  m_Offscreen._ShadowCmdBuffers.resize(m_TotalCmdBuffers);
  m_HDR._CmdBuffers.resize(m_TotalCmdBuffers);
  m_Pbr._CmdBuffers.resize(m_TotalCmdBuffers);

  m_Downscale._Horizontal = 0;
  m_Downscale._Strength = 1.0f;
  m_Downscale._Scale = 1.0f;
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
}


void Renderer::BeginFrame()
{
  m_Rendering = true;
  m_pRhi->PresentWaitIdle();
  m_pRhi->AcquireNextImage();
}


void Renderer::EndFrame()
{
  m_Rendering = false;
  m_pRhi->Present();
}


void Renderer::Render()
{
  static u32 bPreviousFrame = false;
  // TODO(): Signal a beginning and end callback or so, when performing 
  // any rendering.
  CheckCmdUpdate();

  // TODO(): Need to clean this up.
  VkCommandBuffer offscreen_CmdBuffers[3] = { 
    m_Offscreen._CmdBuffers[CurrentCmdBufferIdx()]->Handle(), 
    nullptr,
    nullptr
  };
  VkSemaphore offscreen_WaitSemas[] = { m_pRhi->SwapchainObject()->ImageAvailableSemaphore() };
  VkSemaphore offscreen_SignalSemas[] = { m_Offscreen._Semaphore->Handle() };

  VkCommandBuffer pbr_CmdBuffers[] = { m_Pbr._CmdBuffers[CurrentCmdBufferIdx()]->Handle() };
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

  // Postprocessing, HDR waits for skybox to finish rendering onto scene.
  VkSubmitInfo hdrSI = offscreenSI;
  VkSemaphore hdr_SignalSemas[] = { m_HDR._Semaphore->Handle() };
  VkCommandBuffer hdrCmd = m_HDR._CmdBuffers[CurrentCmdBufferIdx()]->Handle();
  hdrSI.pCommandBuffers = &hdrCmd;
  hdrSI.pSignalSemaphores = hdr_SignalSemas;
  hdrSI.pWaitSemaphores = skybox_SignalSemas;

  VkSemaphore  hdr_Sema = m_HDR._Semaphore->Handle();
  VkSemaphore* final_WaitSemas = &hdr_Sema;
  if (!m_HDR._Enabled) final_WaitSemas = skybox_SignalSemas;

  // Update materials before rendering the frame.
  UpdateMaterials();

  // Wait for fences before starting next frame.
  if (bPreviousFrame) {
    vkWaitForFences(m_pRhi->LogicDevice()->Native(), 1, &cpuFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_pRhi->LogicDevice()->Native(), 1, &cpuFence);
  }

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

    VkSubmitInfo submits[] { offscreenSI, pbrSi, skyboxSI };
    // Submit to renderqueue.
    m_pRhi->GraphicsSubmit(3, submits);

    // High Dynamic Range and Gamma Pass.
    if (m_HDR._Enabled) m_pRhi->GraphicsSubmit(1, &hdrSI);

    // Before calling this cmd buffer, we want to submit our offscreen buffer first, then
    // sent our signal to our swapchain cmd buffers.
  
    // TODO(): We want to hold off on signalling GraphicsFinished Semaphore, and instead 
    // have it signal the SignalUI semaphore instead. UI Overlay will be the one to use
    // GraphicsFinished Semaphore to signal end of frame rendering.
    VkSemaphore signal = m_pRhi->GraphicsFinishedSemaphore();
    VkSemaphore uiSig = m_pUI->Signal()->Handle();
    m_pRhi->SubmitCurrSwapchainCmdBuffer(1, final_WaitSemas, 1, &signal, cpuFence); // cpuFence will need to wait until overlay is finished.

    // Render the Overlay.
    RenderOverlay();
    bPreviousFrame = true;
  EndFrame();


  // Compute pipeline render.
  VkSubmitInfo computeSubmit = { };
  computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  computeSubmit.commandBufferCount = 0;
  computeSubmit.pCommandBuffers = nullptr;
  computeSubmit.signalSemaphoreCount = 0;
  computeSubmit.waitSemaphoreCount = 0;

  m_pRhi->ComputeSubmit(computeSubmit);
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

  m_RenderQuad.CleanUp();
  CleanUpPBR();
  CleanUpHDR(true);
  CleanUpDownscale(true);
  CleanUpOffscreen(true);
  CleanUpFinalOutputs();
  CleanUpDescriptorSetLayouts();
  CleanUpGraphicsPipelines();
  CleanUpFrameBuffers();
  CleanUpRenderTextures(true);

  vkDestroyFence(m_pRhi->LogicDevice()->Native(), cpuFence, nullptr);

  if (m_pRhi) {
    m_pRhi->CleanUp();
    delete m_pRhi;
    m_pRhi = nullptr;
  }
  m_Initialized = false;
}


b8 Renderer::Initialize(Window* window)
{
  if (!window) return false;
  if (m_Initialized) return true;
  
  m_pWindow = window;
  m_pRhi->Initialize(window->Handle());

  SetUpRenderTextures(true);
  SetUpFrameBuffers();
  SetUpDescriptorSetLayouts();
  m_RenderQuad.Initialize(m_pRhi);

  GlobalDescriptor* gMat = new GlobalDescriptor();
  gMat->m_pRhi = m_pRhi;
  gMat->Initialize();
  gMat->Update();
  m_pGlobal = gMat;

  m_pSky = new Sky();
  m_pSky->Initialize();
  m_pSky->MarkDirty();

  SetUpSkybox();
  SetUpGraphicsPipelines();
  SetUpFinalOutputs();
  SetUpOffscreen(true);
  SetUpDownscale(true);
  SetUpHDR(true);
  SetUpPBR();

  m_pLights = new LightDescriptor();
  m_pLights->m_pRhi = m_pRhi;
  m_pLights->Initialize();
  m_pLights->Update();

  VkFenceCreateInfo fenceCi = { };
  fenceCi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  
  vkCreateFence(m_pRhi->LogicDevice()->Native(), &fenceCi, nullptr, &cpuFence);

  m_pRhi->SetSwapchainCmdBufferBuild([&] (CommandBuffer& cmdBuffer, VkRenderPassBeginInfo& defaultRenderpass) -> void {
    // Do stuff with the buffer.
    VkViewport viewport = { };
    viewport.height = (r32) m_pWindow->Height();
    viewport.width = (r32) m_pWindow->Width();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    
    GraphicsPipeline* finalPipeline = gResources().GetGraphicsPipeline(FinalPipelineStr);
    DescriptorSet* finalSet = gResources().GetDescriptorSet(FinalDescSetStr);
    
    cmdBuffer.BeginRenderPass(defaultRenderpass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer.SetViewPorts(0, 1, &viewport);
      cmdBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Pipeline());
      VkDescriptorSet finalDescriptorSets[] = { finalSet->Handle() };    

      cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Layout(), 0, 1, finalDescriptorSets, 0, nullptr);
      VkBuffer vertexBuffer = m_RenderQuad.Quad()->Handle()->NativeBuffer();
      VkBuffer indexBuffer = m_RenderQuad.Indices()->Handle()->NativeBuffer();
      VkDeviceSize offsets[] = { 0 };

      cmdBuffer.BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer.BindVertexBuffers(0, 1, &vertexBuffer, offsets);

      cmdBuffer.DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer.EndRenderPass();
  });

  if (!m_pUI) {
    m_pUI = new UIOverlay();
    m_pUI->Initialize(m_pRhi);
  }

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
    gResources().RegisterDescriptorSetLayout(LightViewDescriptorSetLayoutStr, LightViewLayout);  
  }

  DescriptorSetLayout* GlobalSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* MeshSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* MaterialSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* LightSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* BonesSetLayout = m_pRhi->CreateDescriptorSetLayout();
  DescriptorSetLayout* SkySetLayout = m_pRhi->CreateDescriptorSetLayout();
  gResources().RegisterDescriptorSetLayout(GlobalSetLayoutStr, GlobalSetLayout);
  gResources().RegisterDescriptorSetLayout(MeshSetLayoutStr, MeshSetLayout);
  gResources().RegisterDescriptorSetLayout(MaterialSetLayoutStr, MaterialSetLayout);
  gResources().RegisterDescriptorSetLayout(LightSetLayoutStr, LightSetLayout);
  gResources().RegisterDescriptorSetLayout(BonesSetLayoutStr, BonesSetLayout);
  gResources().RegisterDescriptorSetLayout(SkyboxSetLayoutStr, SkySetLayout);

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
    std::array<VkDescriptorSetLayoutBinding, 7> MaterialBindings;
    // MaterialDescriptor Buffer
    MaterialBindings[6].binding = 0;
    MaterialBindings[6].descriptorCount = 1;
    MaterialBindings[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    MaterialBindings[6].pImmutableSamplers = nullptr;
    MaterialBindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // albedo
    MaterialBindings[0].binding = 1;
    MaterialBindings[0].descriptorCount = 1;
    MaterialBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[0].pImmutableSamplers = nullptr;
    MaterialBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // metallic
    MaterialBindings[1].binding = 2;
    MaterialBindings[1].descriptorCount = 1;
    MaterialBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[1].pImmutableSamplers = nullptr;
    MaterialBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // roughness
    MaterialBindings[2].binding = 3;
    MaterialBindings[2].descriptorCount = 1;
    MaterialBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[2].pImmutableSamplers = nullptr;
    MaterialBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // normal
    MaterialBindings[3].binding = 4;
    MaterialBindings[3].descriptorCount = 1;
    MaterialBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[3].pImmutableSamplers = nullptr;
    MaterialBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // ao
    MaterialBindings[4].binding = 5;
    MaterialBindings[4].descriptorCount = 1;
    MaterialBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[4].pImmutableSamplers = nullptr;
    MaterialBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // emissive
    MaterialBindings[5].binding = 6;
    MaterialBindings[5].descriptorCount = 1;
    MaterialBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialBindings[5].pImmutableSamplers = nullptr;
    MaterialBindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo MaterialLayout = {};
    MaterialLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    MaterialLayout.bindingCount = static_cast<u32>(MaterialBindings.size());
    MaterialLayout.pBindings = MaterialBindings.data();

    MaterialSetLayout->Initialize(MaterialLayout);
  }

  // PBR descriptor layout.
  {
    DescriptorSetLayout* pbr_Layout = m_pRhi->CreateDescriptorSetLayout();
    gResources().RegisterDescriptorSetLayout(pbr_DescLayoutStr, pbr_Layout);
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

    // Rough Metal
    bindings[3].binding = 3;
    bindings[3].descriptorCount = 1;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].pImmutableSamplers = nullptr;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Emission
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
  // Final Layout pass.
  {
    DescriptorSetLayout* finalSetLayout = m_pRhi->CreateDescriptorSetLayout();
    gResources().RegisterDescriptorSetLayout(FinalDescSetLayoutStr, finalSetLayout);
  
    std::array<VkDescriptorSetLayoutBinding, 2> finalBindings;

    finalBindings[0].binding = 0;
    finalBindings[0].descriptorCount = 1;
    finalBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    finalBindings[0].pImmutableSamplers = nullptr;
    finalBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    finalBindings[1].binding = 1;
    finalBindings[1].descriptorCount = 1;
    finalBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    finalBindings[1].pImmutableSamplers = nullptr;
    finalBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo finalLayoutInfo = {};
    finalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    finalLayoutInfo.bindingCount = static_cast<u32>(finalBindings.size());
    finalLayoutInfo.pBindings = finalBindings.data();

    finalSetLayout->Initialize(finalLayoutInfo);
  }
  // HDR Layout pass.
  DescriptorSetLayout* hdrSetLayout = m_pRhi->CreateDescriptorSetLayout();
  gResources().RegisterDescriptorSetLayout(HDRGammaDescSetLayoutStr, hdrSetLayout);
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
  gResources().RegisterDescriptorSetLayout(DownscaleBlurLayoutStr, downscaleLayout);

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
  gResources().RegisterDescriptorSetLayout(GlowDescriptorSetLayoutStr, GlowLayout);
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
  DescriptorSetLayout* GlobalSetLayout = gResources().UnregisterDescriptorSetLayout(GlobalSetLayoutStr);
  DescriptorSetLayout* MeshSetLayout = gResources().UnregisterDescriptorSetLayout(MeshSetLayoutStr);
  DescriptorSetLayout* MaterialSetLayout = gResources().UnregisterDescriptorSetLayout(MaterialSetLayoutStr);
  DescriptorSetLayout* LightSetLayout = gResources().UnregisterDescriptorSetLayout(LightSetLayoutStr);
  DescriptorSetLayout* BonesSetLayout = gResources().UnregisterDescriptorSetLayout(BonesSetLayoutStr);
  DescriptorSetLayout* SkySetLayout = gResources().UnregisterDescriptorSetLayout(SkyboxSetLayoutStr);

  m_pRhi->FreeDescriptorSetLayout(GlobalSetLayout);
  m_pRhi->FreeDescriptorSetLayout(MeshSetLayout);
  m_pRhi->FreeDescriptorSetLayout(MaterialSetLayout);
  m_pRhi->FreeDescriptorSetLayout(LightSetLayout);
  m_pRhi->FreeDescriptorSetLayout(BonesSetLayout);
  m_pRhi->FreeDescriptorSetLayout(SkySetLayout);

  DescriptorSetLayout* LightViewLayout = gResources().UnregisterDescriptorSetLayout(LightViewDescriptorSetLayoutStr);
  m_pRhi->FreeDescriptorSetLayout(LightViewLayout);

  DescriptorSetLayout* finalSetLayout = gResources().UnregisterDescriptorSetLayout(FinalDescSetLayoutStr);
  m_pRhi->FreeDescriptorSetLayout(finalSetLayout);

  DescriptorSetLayout* hdrSetLayout = gResources().UnregisterDescriptorSetLayout(HDRGammaDescSetLayoutStr);
  m_pRhi->FreeDescriptorSetLayout(hdrSetLayout);

  DescriptorSetLayout* downscaleLayout = gResources().UnregisterDescriptorSetLayout(DownscaleBlurLayoutStr);
  m_pRhi->FreeDescriptorSetLayout(downscaleLayout);

  DescriptorSetLayout* GlowLayout = gResources().UnregisterDescriptorSetLayout(GlowDescriptorSetLayoutStr);
  m_pRhi->FreeDescriptorSetLayout(GlowLayout);

  DescriptorSetLayout* PbrLayout = gResources().UnregisterDescriptorSetLayout(pbr_DescLayoutStr);
  m_pRhi->FreeDescriptorSetLayout(PbrLayout);
}


void Renderer::SetUpFrameBuffers()
{
  Texture* gbuffer_Albedo = gResources().GetRenderTexture(gbuffer_AlbedoAttachStr);
  Texture* gbuffer_Normal = gResources().GetRenderTexture(gbuffer_NormalAttachStr);
  Texture* gbuffer_Position = gResources().GetRenderTexture(gbuffer_PositionAttachStr);
  Texture* gbuffer_RoughMetal = gResources().GetRenderTexture(gbuffer_RoughMetalAttachStr);
  Texture* gbuffer_Emission = gResources().GetRenderTexture(gbuffer_EmissionAttachStr);
  Texture* gbuffer_Depth = gResources().GetRenderTexture(gbuffer_DepthAttachStr);

  FrameBuffer* gbuffer_FrameBuffer = m_pRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer(gbuffer_FrameBufferStr, gbuffer_FrameBuffer);

  FrameBuffer* pbr_FrameBuffer = m_pRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer(pbr_FrameBufferStr, pbr_FrameBuffer);

  FrameBuffer* hdrFrameBuffer = m_pRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer(HDRGammaFrameBufferStr, hdrFrameBuffer);

  std::array<VkAttachmentDescription, 6> attachmentDescriptions;
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
    gbuffer_RoughMetal->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_RoughMetal->Samples()
  );

  attachmentDescriptions[4] = CreateAttachmentDescription(
    gbuffer_Emission->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Emission->Samples()
  );

  attachmentDescriptions[5] = CreateAttachmentDescription(
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

  std::array<VkAttachmentReference, 5> attachmentColors;
  VkAttachmentReference attachmentDepthRef = { static_cast<u32>(attachmentColors.size()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
  attachmentColors[0].attachment = 0;
  attachmentColors[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[1].attachment = 1;
  attachmentColors[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[2].attachment = 2;
  attachmentColors[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[3].attachment = 3;
  attachmentColors[3].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[4].attachment = 4;
  attachmentColors[4].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

  std::array<VkImageView, 6> attachments;
  attachments[0] = gbuffer_Albedo->View();
  attachments[1] = gbuffer_Normal->View();
  attachments[2] = gbuffer_Position->View();
  attachments[3] = gbuffer_RoughMetal->View();
  attachments[4] = gbuffer_Emission->View();
  attachments[5] = gbuffer_Depth->View();

  VkFramebufferCreateInfo framebufferCI = CreateFrameBufferInfo(
    m_pWindow->Width(),
    m_pWindow->Height(),
    nullptr, // Finalize() call handles this for us.
    static_cast<u32>(attachments.size()),
    attachments.data(),
    1
  );

  gbuffer_FrameBuffer->Finalize(framebufferCI, renderpassCI);

  // pbr framebuffer.
  {
    Texture* pbr_Bright = gResources().GetRenderTexture(pbr_BrightTextureStr);
    Texture* pbr_Final = gResources().GetRenderTexture(pbr_FinalTextureStr);
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
      m_pWindow->Width(),
      m_pWindow->Height(),
      nullptr, // Finalize() call handles this for us.
      static_cast<u32>(pbrAttachments.size()),
      pbrAttachments.data(),
      1
    );

    pbr_FrameBuffer->Finalize(pbrFramebufferCI, pbrRenderpassCI);
  }
  
  // No need to render any depth, as we are only writing on a 2d surface.
  Texture* hdrColor = gResources().GetRenderTexture(HDRGammaColorAttachStr);
  subpass.pDepthStencilAttachment = nullptr;
  attachments[0] = hdrColor->View();
  framebufferCI.attachmentCount = 1;
  attachmentDescriptions[0].format = hdrColor->Format();
  attachmentDescriptions[0].samples = hdrColor->Samples();
  renderpassCI.attachmentCount = 1;
  subpass.colorAttachmentCount = 1;
  hdrFrameBuffer->Finalize(framebufferCI, renderpassCI);

  // Downscale render textures.
  Texture* rtDownScale2x = gResources().GetRenderTexture(RenderTarget2xHorizStr);
  Texture* RenderTarget2xFinal = gResources().GetRenderTexture(RenderTarget2xFinalStr);
  Texture* rtDownScale4x = gResources().GetRenderTexture(RenderTarget4xScaledStr);
  Texture* RenderTarget4xFinal = gResources().GetRenderTexture(RenderTarget4xFinalStr);
  Texture* rtDownScale8x = gResources().GetRenderTexture(RenderTarget8xScaledStr);
  Texture* RenderTarget8xFinal = gResources().GetRenderTexture(RenderTarget8xFinalStr);
  Texture* rtDownScale16x = gResources().GetRenderTexture(RenderTarget16xScaledStr);
  Texture* RenderTarget16xFinal = gResources().GetRenderTexture(RenderTarget16xFinalStr);
  Texture* GlowTarget = gResources().GetRenderTexture(RenderTargetGlowStr);

  FrameBuffer* DownScaleFB2x = m_pRhi->CreateFrameBuffer();
  FrameBuffer* FB2xFinal = m_pRhi->CreateFrameBuffer();
  FrameBuffer* DownScaleFB4x = m_pRhi->CreateFrameBuffer();
  FrameBuffer* FB4xFinal = m_pRhi->CreateFrameBuffer();
  FrameBuffer* DownScaleFB8x = m_pRhi->CreateFrameBuffer();
  FrameBuffer* FB8xFinal = m_pRhi->CreateFrameBuffer();
  FrameBuffer* DownScaleFB16x = m_pRhi->CreateFrameBuffer();
  FrameBuffer* FB16xFinal = m_pRhi->CreateFrameBuffer();
  FrameBuffer* GlowFB = m_pRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer(FrameBuffer2xHorizStr, DownScaleFB2x);
  gResources().RegisterFrameBuffer(FrameBuffer2xFinalStr, FB2xFinal);
  gResources().RegisterFrameBuffer(FrameBuffer4xStr, DownScaleFB4x); 
  gResources().RegisterFrameBuffer(FrameBuffer4xFinalStr, FB4xFinal);
  gResources().RegisterFrameBuffer(FrameBuffer8xStr, DownScaleFB8x);
  gResources().RegisterFrameBuffer(FrameBuffer8xFinalStr, FB8xFinal);
  gResources().RegisterFrameBuffer(FrameBuffer16xStr, DownScaleFB16x);
  gResources().RegisterFrameBuffer(FrameBuffer16xFinalStr, FB16xFinal);
  gResources().RegisterFrameBuffer(FrameBufferGlowStr, GlowFB);

  // 2x
  attachments[0] = RenderTarget2xFinal->View();
  attachmentDescriptions[0].format = RenderTarget2xFinal->Format();
  attachmentDescriptions[0].samples = RenderTarget2xFinal->Samples();
  framebufferCI.width = RenderTarget2xFinal->Width();
  framebufferCI.height = RenderTarget2xFinal->Height();
  FB2xFinal->Finalize(framebufferCI, renderpassCI);

  attachments[0] = rtDownScale2x->View();
  attachmentDescriptions[0].format = rtDownScale2x->Format();
  attachmentDescriptions[0].samples = rtDownScale2x->Samples();
  framebufferCI.width = rtDownScale2x->Width();
  framebufferCI.height = rtDownScale2x->Height();
  DownScaleFB2x->Finalize(framebufferCI, renderpassCI);

  // 4x
  attachments[0] = RenderTarget4xFinal->View();
  attachmentDescriptions[0].format = RenderTarget4xFinal->Format();
  attachmentDescriptions[0].samples = RenderTarget4xFinal->Samples();
  framebufferCI.width = RenderTarget4xFinal->Width();
  framebufferCI.height = RenderTarget4xFinal->Height();
  FB4xFinal->Finalize(framebufferCI, renderpassCI);

  attachments[0] = rtDownScale4x->View();
  attachmentDescriptions[0].format = rtDownScale4x->Format();
  attachmentDescriptions[0].samples = rtDownScale4x->Samples();
  framebufferCI.width = rtDownScale4x->Width();
  framebufferCI.height = rtDownScale4x->Height();
  DownScaleFB4x->Finalize(framebufferCI, renderpassCI);

  // 8x
  attachments[0] = RenderTarget8xFinal->View();
  attachmentDescriptions[0].format = RenderTarget8xFinal->Format();
  attachmentDescriptions[0].samples = RenderTarget8xFinal->Samples();
  framebufferCI.width = RenderTarget8xFinal->Width();
  framebufferCI.height = RenderTarget8xFinal->Height();
  FB8xFinal->Finalize(framebufferCI, renderpassCI);

  attachments[0] = rtDownScale8x->View();
  attachmentDescriptions[0].format = rtDownScale8x->Format();
  attachmentDescriptions[0].samples = rtDownScale8x->Samples();
  framebufferCI.width = rtDownScale8x->Width();
  framebufferCI.height = rtDownScale8x->Height();
  DownScaleFB8x->Finalize(framebufferCI, renderpassCI);

  // 16x
  attachments[0] = RenderTarget16xFinal->View();
  attachmentDescriptions[0].format = RenderTarget16xFinal->Format();
  attachmentDescriptions[0].samples = RenderTarget16xFinal->Samples();
  framebufferCI.width = RenderTarget16xFinal->Width();
  framebufferCI.height = RenderTarget16xFinal->Height();
  FB16xFinal->Finalize(framebufferCI, renderpassCI);

  attachments[0] = rtDownScale16x->View();
  attachmentDescriptions[0].format = rtDownScale16x->Format();
  attachmentDescriptions[0].samples = rtDownScale16x->Samples();
  framebufferCI.width = rtDownScale16x->Width();
  framebufferCI.height = rtDownScale16x->Height();
  DownScaleFB16x->Finalize(framebufferCI, renderpassCI);


  // Glow
  attachments[0] = GlowTarget->View();
  attachmentDescriptions[0].format = GlowTarget->Format();
  attachmentDescriptions[0].samples = GlowTarget->Samples();
  framebufferCI.width = m_pRhi->SwapchainObject()->SwapchainExtent().width;
  framebufferCI.height = m_pRhi->SwapchainObject()->SwapchainExtent().height;
  GlowFB->Finalize(framebufferCI, renderpassCI);
}


void Renderer::SetUpGraphicsPipelines()
{
  std::string Filepath = gFilesystem().CurrentAppDirectory();

  VkPipelineInputAssemblyStateCreateInfo assemblyCI = { };
  assemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  assemblyCI.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = { };
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.height = static_cast<r32>(m_pRhi->SwapchainObject()->SwapchainExtent().height);
  viewport.width = static_cast<r32>(m_pRhi->SwapchainObject()->SwapchainExtent().width);

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
      VK_CULL_MODE_BACK_BIT,
      VK_FRONT_FACE_CLOCKWISE,
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

  std::array<VkPipelineColorBlendAttachmentState, 5> colorBlendAttachments;
  colorBlendAttachments[0] = CreateColorBlendAttachmentState(
    VK_TRUE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[1] = CreateColorBlendAttachmentState(
    VK_TRUE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[2] = CreateColorBlendAttachmentState(
    VK_TRUE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[3] = CreateColorBlendAttachmentState(
    VK_TRUE,
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_OP_ADD,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_OP_ADD
  );

  colorBlendAttachments[4] = CreateColorBlendAttachmentState(
    VK_TRUE,
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
    GraphicsPipelineInfo.renderPass = m_pLights->m_pFrameBuffer->RenderPass();
    RendererPass::SetUpDirectionalShadowPass(RHI(), Filepath, GraphicsPipelineInfo);
    GraphicsPipelineInfo.renderPass = nullptr;
    viewportCI.pScissors = &scissor;
    colorBlendCI.attachmentCount = static_cast<u32>(static_cast<u32>(colorBlendAttachments.size()));
  } else {
    R_DEBUG(rVerbose, "No framebuffer initialized in light data. Skipping shadow map pass...\n");
  }
    
  RendererPass::SetUpGBufferPass(RHI(), Filepath, GraphicsPipelineInfo);
  RendererPass::SetUpSkyboxPass(RHI(), Filepath, GraphicsPipelineInfo);

  // Set to quad rendering format.
  colorBlendCI.logicOpEnable = VK_FALSE;
  depthStencilCI.depthTestEnable = VK_FALSE;
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

  RendererPass::SetUpPhysicallyBasedPass(RHI(), Filepath, GraphicsPipelineInfo);
  RendererPass::SetUpDownScalePass(RHI(), Filepath, GraphicsPipelineInfo);
  RendererPass::SetUpHDRGammaPass(RHI(), Filepath, GraphicsPipelineInfo);
  colorBlendAttachments[0].blendEnable = VK_FALSE;
  RendererPass::SetUpFinalPass(RHI(), Filepath, GraphicsPipelineInfo);
}


void Renderer::CleanUpGraphicsPipelines()
{
  GraphicsPipeline* gbuffer_Pipeline = gResources().UnregisterGraphicsPipeline(gbuffer_PipelineStr);
  m_pRhi->FreeGraphicsPipeline(gbuffer_Pipeline);

  GraphicsPipeline* pbr_Pipeline = gResources().UnregisterGraphicsPipeline(pbr_PipelineStr);
  m_pRhi->FreeGraphicsPipeline(pbr_Pipeline);

  GraphicsPipeline* gbuffer_StaticPipeline = gResources().UnregisterGraphicsPipeline(gbuffer_StaticPipelineStr);
  m_pRhi->FreeGraphicsPipeline(gbuffer_StaticPipeline);

  GraphicsPipeline* QuadPipeline = gResources().UnregisterGraphicsPipeline(FinalPipelineStr);
  m_pRhi->FreeGraphicsPipeline(QuadPipeline);

  GraphicsPipeline* HdrPipeline = gResources().UnregisterGraphicsPipeline(HDRGammaPipelineStr);
  m_pRhi->FreeGraphicsPipeline(HdrPipeline);

  GraphicsPipeline* DownscalePipeline2x = gResources().UnregisterGraphicsPipeline(DownscaleBlurPipeline2xStr);
  m_pRhi->FreeGraphicsPipeline(DownscalePipeline2x);

  GraphicsPipeline* DownscalePipeline4x = gResources().UnregisterGraphicsPipeline(DownscaleBlurPipeline4xStr);
  m_pRhi->FreeGraphicsPipeline(DownscalePipeline4x);

  GraphicsPipeline* DownscalePipeline8x = gResources().UnregisterGraphicsPipeline(DownscaleBlurPipeline8xStr);
  m_pRhi->FreeGraphicsPipeline(DownscalePipeline8x);

  GraphicsPipeline* DownscalePipeline16x = gResources().UnregisterGraphicsPipeline(DownscaleBlurPipeline16xStr);
  m_pRhi->FreeGraphicsPipeline(DownscalePipeline16x);

  GraphicsPipeline* GlowPipeline = gResources().UnregisterGraphicsPipeline(GlowPipelineStr);
  m_pRhi->FreeGraphicsPipeline(GlowPipeline);

  GraphicsPipeline* SkyPipeline = gResources().UnregisterGraphicsPipeline(SkyboxPipelineStr);
  m_pRhi->FreeGraphicsPipeline(SkyPipeline);

  GraphicsPipeline* ShadowMapPipeline = gResources().UnregisterGraphicsPipeline(ShadowMapPipelineStr);
  GraphicsPipeline* DynamicShadowMapPipline = gResources().UnregisterGraphicsPipeline(DynamicShadowMapPipelineStr);
  if (ShadowMapPipeline) {
    m_pRhi->FreeGraphicsPipeline(ShadowMapPipeline);
  }
  
  if (DynamicShadowMapPipline) {
    m_pRhi->FreeGraphicsPipeline(DynamicShadowMapPipline);
  }
}


void Renderer::CleanUpFrameBuffers()
{
  FrameBuffer* gbuffer_FrameBuffer = gResources().UnregisterFrameBuffer(gbuffer_FrameBufferStr);
  m_pRhi->FreeFrameBuffer(gbuffer_FrameBuffer);

  FrameBuffer* pbr_FrameBuffer = gResources().UnregisterFrameBuffer(pbr_FrameBufferStr);
  m_pRhi->FreeFrameBuffer(pbr_FrameBuffer);

  FrameBuffer* hdrFrameBuffer = gResources().UnregisterFrameBuffer(HDRGammaFrameBufferStr);
  m_pRhi->FreeFrameBuffer(hdrFrameBuffer);

  FrameBuffer* DownScaleFB2x = gResources().UnregisterFrameBuffer(FrameBuffer2xHorizStr);
  FrameBuffer* FB2xFinal = gResources().UnregisterFrameBuffer(FrameBuffer2xFinalStr);
  FrameBuffer* DownScaleFB4x = gResources().UnregisterFrameBuffer(FrameBuffer4xStr);
  FrameBuffer* FB4xFinal = gResources().UnregisterFrameBuffer(FrameBuffer4xFinalStr);
  FrameBuffer* DownScaleFB8x = gResources().UnregisterFrameBuffer(FrameBuffer8xStr);
  FrameBuffer* FB8xFinal = gResources().UnregisterFrameBuffer(FrameBuffer8xFinalStr);
  FrameBuffer* DownScaleFB16x = gResources().UnregisterFrameBuffer(FrameBuffer16xStr);
  FrameBuffer* FB16xFinal = gResources().UnregisterFrameBuffer(FrameBuffer16xFinalStr);
  FrameBuffer* GlowFB = gResources().UnregisterFrameBuffer(FrameBufferGlowStr);

  m_pRhi->FreeFrameBuffer(DownScaleFB2x);
  m_pRhi->FreeFrameBuffer(DownScaleFB4x);
  m_pRhi->FreeFrameBuffer(DownScaleFB8x);
  m_pRhi->FreeFrameBuffer(DownScaleFB16x);
  m_pRhi->FreeFrameBuffer(FB2xFinal);
  m_pRhi->FreeFrameBuffer(FB4xFinal);
  m_pRhi->FreeFrameBuffer(FB8xFinal);
  m_pRhi->FreeFrameBuffer(FB16xFinal);
  m_pRhi->FreeFrameBuffer(GlowFB);
}


void Renderer::SetUpRenderTextures(b8 fullSetup)
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
  Texture* gbuffer_Position = m_pRhi->CreateTexture();
  Texture* gbuffer_RoughMetal = m_pRhi->CreateTexture();
  Texture* gbuffer_Emission = m_pRhi->CreateTexture();
  Texture* gbuffer_Depth = m_pRhi->CreateTexture();
  Sampler* gbuffer_Sampler = m_pRhi->CreateSampler();

  Texture* hdr_Texture = m_pRhi->CreateTexture();
  Sampler* hdr_Sampler = m_pRhi->CreateSampler();

  gResources().RegisterSampler(HDRGammaSamplerStr, hdr_Sampler);
  gResources().RegisterRenderTexture(HDRGammaColorAttachStr, hdr_Texture);
  gResources().RegisterRenderTexture(gbuffer_AlbedoAttachStr, gbuffer_Albedo);
  gResources().RegisterRenderTexture(gbuffer_NormalAttachStr, gbuffer_Normal);
  gResources().RegisterRenderTexture(gbuffer_PositionAttachStr, gbuffer_Position);
  gResources().RegisterRenderTexture(gbuffer_RoughMetalAttachStr, gbuffer_RoughMetal);
  gResources().RegisterRenderTexture(gbuffer_EmissionAttachStr, gbuffer_Emission);
  gResources().RegisterRenderTexture(gbuffer_DepthAttachStr, gbuffer_Depth);
  gResources().RegisterRenderTexture(pbr_FinalTextureStr, pbr_Final);
  gResources().RegisterRenderTexture(pbr_BrightTextureStr, pbr_Bright);
  gResources().RegisterRenderTexture(RenderTarget2xHorizStr, renderTarget2xScaled);
  gResources().RegisterRenderTexture(RenderTarget2xFinalStr, RenderTarget2xFinal);
  gResources().RegisterRenderTexture(RenderTarget4xScaledStr, renderTarget4xScaled);
  gResources().RegisterRenderTexture(RenderTarget4xFinalStr, RenderTarget4xFinal);
  gResources().RegisterRenderTexture(RenderTarget8xScaledStr, renderTarget8xScaled);
  gResources().RegisterRenderTexture(RenderTarget8xFinalStr, RenderTarget8xFinal);
  gResources().RegisterRenderTexture(RenderTarget16xScaledStr, RenderTarget16xScaled);
  gResources().RegisterRenderTexture(RenderTarget16xFinalStr, RenderTarget16xFinal);
  gResources().RegisterRenderTexture(RenderTargetGlowStr, GlowTarget);
  gResources().RegisterSampler(gbuffer_SamplerStr, gbuffer_Sampler);
  
  VkImageCreateInfo cImageInfo = { };
  VkImageViewCreateInfo cViewInfo = { };

  cImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  cImageInfo.imageType = VK_IMAGE_TYPE_2D;
  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  cImageInfo.mipLevels = 1;
  cImageInfo.extent.depth = 1;
  cImageInfo.arrayLayers = 1;
  cImageInfo.extent.width = m_pWindow->Width();
  cImageInfo.extent.height = m_pWindow->Height();
  cImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  cImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  cImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

  cViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cViewInfo.image = nullptr; // No need to set the image, texture->Initialize() handles this for us.
  cViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  cViewInfo.subresourceRange = { };
  cViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  cViewInfo.subresourceRange.baseArrayLayer = 0;
  cViewInfo.subresourceRange.baseMipLevel = 0;
  cViewInfo.subresourceRange.layerCount = 1;
  cViewInfo.subresourceRange.levelCount = 1;

  gbuffer_Albedo->Initialize(cImageInfo, cViewInfo);
  gbuffer_Normal->Initialize(cImageInfo, cViewInfo);
  pbr_Bright->Initialize(cImageInfo, cViewInfo);
  pbr_Final->Initialize(cImageInfo, cViewInfo);
  GlowTarget->Initialize(cImageInfo, cViewInfo);

  // Initialize downscaled render textures.
  cImageInfo.extent.width = m_pWindow->Width()    >> 1;
  cImageInfo.extent.height = m_pWindow->Height()  >> 1;
  renderTarget2xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget2xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = m_pWindow->Width()    >> 2;
  cImageInfo.extent.height = m_pWindow->Height()  >> 2;
  renderTarget4xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget4xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = m_pWindow->Width()    >> 3;
  cImageInfo.extent.height = m_pWindow->Height()  >> 3;
  renderTarget8xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget8xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = m_pWindow->Width()    >> 4;
  cImageInfo.extent.height = m_pWindow->Height()  >> 4;
  RenderTarget16xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget16xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  cViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  cImageInfo.extent.width = m_pWindow->Width();
  cImageInfo.extent.height = m_pWindow->Height();
  gbuffer_Position->Initialize(cImageInfo, cViewInfo);
  gbuffer_RoughMetal->Initialize(cImageInfo, cViewInfo);
  gbuffer_Emission->Initialize(cImageInfo, cViewInfo);

  // Depth attachment texture.
  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.extent.width = m_pWindow->Width();
  cImageInfo.extent.height = m_pWindow->Height();
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  hdr_Texture->Initialize(cImageInfo, cViewInfo);

  cImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  cImageInfo.extent.width = m_pWindow->Width();
  cImageInfo.extent.height = m_pWindow->Height();
  cViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
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
  samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerCI.maxLod = 1.0f;
  samplerCI.minLod = 0.0f;
  samplerCI.unnormalizedCoordinates = VK_FALSE;

  gbuffer_Sampler->Initialize(samplerCI);
  hdr_Sampler->Initialize(samplerCI);
  if (fullSetup) {
    Sampler* defaultSampler = m_pRhi->CreateSampler();
    defaultSampler->Initialize(samplerCI);
    gResources().RegisterSampler(DefaultSamplerStr, defaultSampler);

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
    dImageInfo.extent.width = m_pWindow->Width();
    dImageInfo.extent.height = m_pWindow->Height();
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
    gResources().RegisterRenderTexture(DefaultTextureStr, defaultTexture);
  }
}


void Renderer::CleanUpRenderTextures(b8 fullCleanup)
{
  Texture* renderTarget2xScaled = gResources().UnregisterRenderTexture(RenderTarget2xHorizStr);
  Texture* RenderTarget2xFinal = gResources().UnregisterRenderTexture(RenderTarget2xFinalStr);
  Texture* renderTarget4xScaled = gResources().UnregisterRenderTexture(RenderTarget4xScaledStr);
  Texture* RenderTarget4xFinal = gResources().UnregisterRenderTexture(RenderTarget4xFinalStr);
  Texture* renderTarget8xScaled = gResources().UnregisterRenderTexture(RenderTarget8xScaledStr);
  Texture* RenderTarget8xFinal = gResources().UnregisterRenderTexture(RenderTarget8xFinalStr);
  Texture* RenderTarget16xScaled = gResources().UnregisterRenderTexture(RenderTarget16xScaledStr);
  Texture* RenderTarget16xFinal = gResources().UnregisterRenderTexture(RenderTarget16xFinalStr);
  Texture* GlowTarget = gResources().UnregisterRenderTexture(RenderTargetGlowStr);

  m_pRhi->FreeTexture(renderTarget2xScaled);
  m_pRhi->FreeTexture(RenderTarget2xFinal);
  m_pRhi->FreeTexture(renderTarget4xScaled);
  m_pRhi->FreeTexture(RenderTarget4xFinal);
  m_pRhi->FreeTexture(RenderTarget8xFinal);
  m_pRhi->FreeTexture(renderTarget8xScaled);
  m_pRhi->FreeTexture(RenderTarget16xScaled);
  m_pRhi->FreeTexture(RenderTarget16xFinal);
  m_pRhi->FreeTexture(GlowTarget);

  Texture* gbuffer_Albedo = gResources().UnregisterRenderTexture(gbuffer_AlbedoAttachStr);
  Texture* gbuffer_Normal = gResources().UnregisterRenderTexture(gbuffer_NormalAttachStr);
  Texture* gbuffer_Position = gResources().UnregisterRenderTexture(gbuffer_PositionAttachStr);
  Texture* gbuffer_RoughMetal = gResources().UnregisterRenderTexture(gbuffer_RoughMetalAttachStr);
  Texture* gbuffer_Emission = gResources().UnregisterRenderTexture(gbuffer_EmissionAttachStr);
  Texture* gbuffer_Depth = gResources().UnregisterRenderTexture(gbuffer_DepthAttachStr);
  Sampler* gbuffer_Sampler = gResources().UnregisterSampler(gbuffer_SamplerStr);

  Texture* pbr_Bright = gResources().UnregisterRenderTexture(pbr_BrightTextureStr);
  Texture* pbr_Final = gResources().UnregisterRenderTexture(pbr_FinalTextureStr);

  Texture* hdr_Texture = gResources().UnregisterRenderTexture(HDRGammaColorAttachStr);
  Sampler* hdr_Sampler = gResources().UnregisterSampler(HDRGammaSamplerStr);
  
  m_pRhi->FreeTexture(hdr_Texture);
  m_pRhi->FreeSampler(hdr_Sampler);

  m_pRhi->FreeTexture(gbuffer_Albedo);
  m_pRhi->FreeTexture(gbuffer_Normal);
  m_pRhi->FreeTexture(gbuffer_Position);
  m_pRhi->FreeTexture(gbuffer_RoughMetal);
  m_pRhi->FreeTexture(gbuffer_Emission);
  m_pRhi->FreeTexture(gbuffer_Depth);
  m_pRhi->FreeSampler(gbuffer_Sampler);
  m_pRhi->FreeTexture(pbr_Bright);
  m_pRhi->FreeTexture(pbr_Final);

  if (fullCleanup) {
    Texture* defaultTexture = gResources().UnregisterRenderTexture(DefaultTextureStr);
    Sampler* defaultSampler = gResources().UnregisterSampler(DefaultSamplerStr);

    m_pRhi->FreeTexture(defaultTexture);
    m_pRhi->FreeSampler(defaultSampler);
  }
}


void Renderer::BuildPbrCmdBuffer(u32 currCmdIdx) 
{
  CommandBuffer* cmdBuffer = m_Pbr._CmdBuffers[currCmdIdx];
  if (cmdBuffer) {
    cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  }

  FrameBuffer* pbr_FrameBuffer = gResources().GetFrameBuffer(pbr_FrameBufferStr);

  VkViewport viewport = {};
  viewport.height = (r32)m_pWindow->Height();
  viewport.width = (r32)m_pWindow->Width();
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
  pbr_RenderPassInfo.renderPass = pbr_FrameBuffer->RenderPass();
  pbr_RenderPassInfo.pClearValues = clearValuesPBR.data();
  pbr_RenderPassInfo.clearValueCount = static_cast<u32>(clearValuesPBR.size());
  pbr_RenderPassInfo.renderArea.extent = m_pRhi->SwapchainObject()->SwapchainExtent();
  pbr_RenderPassInfo.renderArea.offset = { 0, 0 };

  cmdBuffer->Begin(beginInfo);
    cmdBuffer->BeginRenderPass(pbr_RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    GraphicsPipeline* pbr_Pipeline = gResources().GetGraphicsPipeline(pbr_PipelineStr);
    VkDescriptorSet sets[4] = {
      m_pGlobal->Set()->Handle(),
      gResources().GetDescriptorSet(pbr_DescSetStr)->Handle(),
      m_pLights->Set()->Handle(),
      m_pLights->ViewSet()->Handle()
    };
    cmdBuffer->SetViewPorts(0, 1, &viewport);
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_Pipeline->Pipeline());
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_Pipeline->Layout(), 0, 4, sets, 0, nullptr);
    VkBuffer vertexBuffer = m_RenderQuad.Quad()->Handle()->NativeBuffer();
    VkBuffer indexBuffer = m_RenderQuad.Indices()->Handle()->NativeBuffer();
    VkDeviceSize offsets[] = { 0 };
    cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::SetUpOffscreen(b8 fullSetup)
{
  if (fullSetup) {
    m_Offscreen._Semaphore = m_pRhi->CreateVkSemaphore();
    VkSemaphoreCreateInfo semaCI = { };
    semaCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_Offscreen._Semaphore->Initialize(semaCI);

    for (size_t i = 0; i < m_Offscreen._CmdBuffers.size(); ++i) {
      m_Offscreen._CmdBuffers[i] = m_pRhi->CreateCommandBuffer();
      m_Offscreen._CmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
      m_Offscreen._ShadowCmdBuffers[i] = m_pRhi->CreateCommandBuffer();
      m_Offscreen._ShadowCmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);  
  }
  }
}


void Renderer::CleanUpOffscreen(b8 fullCleanup)
{
  if (fullCleanup) {
    m_pRhi->FreeVkSemaphore(m_Offscreen._Semaphore);
    for (size_t i = 0; i < m_Offscreen._CmdBuffers.size(); ++i) {
      m_pRhi->FreeCommandBuffer(m_Offscreen._CmdBuffers[i]);
      m_pRhi->FreeCommandBuffer(m_Offscreen._ShadowCmdBuffers[i]);
    }
  }
}


void Renderer::SetUpDownscale(b8 FullSetUp)
{
  if (FullSetUp) {
  }

  DescriptorSetLayout* Layout = gResources().GetDescriptorSetLayout(DownscaleBlurLayoutStr);
  DescriptorSetLayout* GlowLayout = gResources().GetDescriptorSetLayout(GlowDescriptorSetLayoutStr);
  DescriptorSet* DBDS2x = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS2xFinal = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS4x = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS4xFinal = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS8x = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS8xFinal = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS16x = m_pRhi->CreateDescriptorSet();
  DescriptorSet* DBDS16xFinal = m_pRhi->CreateDescriptorSet();
  DescriptorSet* GlowDS = m_pRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet2x, DBDS2x);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet4x, DBDS4x);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet8x, DBDS8x);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet16x, DBDS16x);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet2xFinalStr, DBDS2xFinal);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet4xFinalStr, DBDS4xFinal);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet8xFinalStr, DBDS8xFinal);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet16xFinalStr, DBDS16xFinal);
  gResources().RegisterDescriptorSet(GlowDescriptorSetStr, GlowDS);

  DBDS2x->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS4x->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS8x->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS16x->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS2xFinal->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS4xFinal->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS8xFinal->Allocate(m_pRhi->DescriptorPool(), Layout);
  DBDS16xFinal->Allocate(m_pRhi->DescriptorPool(), Layout);
  GlowDS->Allocate(m_pRhi->DescriptorPool(), GlowLayout);

  Texture* RTBright = gResources().GetRenderTexture(pbr_BrightTextureStr);
  Texture* Color2x = gResources().GetRenderTexture(RenderTarget2xHorizStr);
  Texture* Color2xFinal = gResources().GetRenderTexture(RenderTarget2xFinalStr);
  Texture* Color4x = gResources().GetRenderTexture(RenderTarget4xScaledStr);
  Texture* Color4xFinal = gResources().GetRenderTexture(RenderTarget4xFinalStr);
  Texture* Color8x = gResources().GetRenderTexture(RenderTarget8xScaledStr);
  Texture* Color8xFinal = gResources().GetRenderTexture(RenderTarget8xFinalStr);
  Texture* Color16x = gResources().GetRenderTexture(RenderTarget16xScaledStr);
  Texture* Color16xFinal = gResources().GetRenderTexture(RenderTarget16xFinalStr);
  Sampler* gbuffer_Sampler = gResources().GetSampler(gbuffer_SamplerStr);
  Sampler* DownscaleSampler = gResources().GetSampler(ScaledSamplerStr);

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


void Renderer::CleanUpDownscale(b8 FullCleanUp)
{
  if (FullCleanUp) {
  }

  DescriptorSet* DBDS2x = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet2x);
  m_pRhi->FreeDescriptorSet(DBDS2x);
  DescriptorSet* DBDS4x = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet4x);
  m_pRhi->FreeDescriptorSet(DBDS4x);
  DescriptorSet* DBDS8x = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet8x);
  m_pRhi->FreeDescriptorSet(DBDS8x);
  DescriptorSet* DBDS16x = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet16x);
  m_pRhi->FreeDescriptorSet(DBDS16x);
  DescriptorSet* DBDS2xFinal = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet2xFinalStr);
  m_pRhi->FreeDescriptorSet(DBDS2xFinal);
  DescriptorSet* DBDS4xFinal = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet4xFinalStr);
  m_pRhi->FreeDescriptorSet(DBDS4xFinal);
  DescriptorSet* DBDS8xFinal = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet8xFinalStr);
  m_pRhi->FreeDescriptorSet(DBDS8xFinal);
  DescriptorSet* DBDS16xFinal = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet16xFinalStr);
  m_pRhi->FreeDescriptorSet(DBDS16xFinal);
  DescriptorSet* GlowDS = gResources().UnregisterDescriptorSet(GlowDescriptorSetStr);
  m_pRhi->FreeDescriptorSet(GlowDS);
}


void Renderer::SetUpHDR(b8 fullSetUp)
{
  if (fullSetUp) {
    for (size_t i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
      m_HDR._CmdBuffers[i] = m_pRhi->CreateCommandBuffer();
      m_HDR._CmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
    m_HDR._Semaphore = m_pRhi->CreateVkSemaphore();
    VkSemaphoreCreateInfo semaCi = { };
    semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_HDR._Semaphore->Initialize(semaCi);
  }

  DescriptorSet* hdrSet = m_pRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet(HDRGammaDescSetStr, hdrSet);
  std::array<VkWriteDescriptorSet, 3> hdrWrites;
  VkDescriptorBufferInfo hdrBufferInfo = {};
  hdrBufferInfo.offset = 0;
  hdrBufferInfo.range = sizeof(GlobalBuffer);
  hdrBufferInfo.buffer = m_pGlobal->Handle()->NativeBuffer();

  VkDescriptorImageInfo pbrImageInfo = { };
  pbrImageInfo.sampler = gResources().GetSampler(gbuffer_SamplerStr)->Handle();
  pbrImageInfo.imageView = gResources().GetRenderTexture(pbr_FinalTextureStr)->View();
  pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // TODO(): We don't have our bloom pipeline and texture yet, we will sub it with this instead!
  VkDescriptorImageInfo bloomImageInfo = { };
  bloomImageInfo.sampler = gResources().GetSampler(gbuffer_SamplerStr)->Handle();
  bloomImageInfo.imageView = gResources().GetRenderTexture(RenderTargetGlowStr)->View();
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
  hdrSet->Allocate(m_pRhi->DescriptorPool(), gResources().GetDescriptorSetLayout(HDRGammaDescSetLayoutStr));
  hdrSet->Update(static_cast<u32>(hdrWrites.size()), hdrWrites.data());
}


void Renderer::CleanUpHDR(b8 fullCleanUp)
{
  if (fullCleanUp) {
    m_pRhi->FreeVkSemaphore(m_HDR._Semaphore);

    for (size_t i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
      m_pRhi->FreeCommandBuffer(m_HDR._CmdBuffers[i]);
      m_HDR._CmdBuffers[i] = nullptr;
    }

    m_HDR._Semaphore = nullptr;
  }

  DescriptorSet* hdrSet = gResources().UnregisterDescriptorSet(HDRGammaDescSetStr);
  m_pRhi->FreeDescriptorSet(hdrSet);
}


void Renderer::Build()
{
  m_pRhi->GraphicsWaitIdle();

  BuildOffScreenBuffer(CurrentCmdBufferIdx());
  BuildHDRCmdBuffer(CurrentCmdBufferIdx());
  BuildShadowCmdBuffer(CurrentCmdBufferIdx());
  BuildPbrCmdBuffer(CurrentCmdBufferIdx());
  BuildSkyboxCmdBuffer();
  m_pRhi->RebuildCommandBuffers(m_pRhi->CurrentSwapchainCmdBufferSet());

  // Signal that no update is required.
  m_NeedsUpdate = false;
  m_AsyncBuild = false;
}


void Renderer::SetUpSkybox()
{
  DescriptorSet* skyboxSet = m_pRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet(SkyboxDescriptorSetStr, skyboxSet);
  DescriptorSetLayout* layout = gResources().GetDescriptorSetLayout(SkyboxSetLayoutStr);

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
  m_pSkyboxCmdBuffer->Allocate(m_pRhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  m_SkyboxFinished = m_pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo sema = { };
  sema.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_SkyboxFinished->Initialize(sema);
}


void Renderer::CleanUpSkybox()
{
  DescriptorSet* skyboxSet = gResources().UnregisterDescriptorSet(SkyboxDescriptorSetStr);
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
  FrameBuffer* skyFrameBuffer = gResources().GetFrameBuffer(pbr_FrameBufferStr);
  GraphicsPipeline* skyPipeline = gResources().GetGraphicsPipeline(SkyboxPipelineStr);
  DescriptorSet* global = m_pGlobal->Set();
  DescriptorSet* skybox = gResources().GetDescriptorSet(SkyboxDescriptorSetStr);

  VkDescriptorSet descriptorSets[] = {
    global->Handle(),
    skybox->Handle()
  };  

  buf->Begin(beginInfo);
    std::array<VkClearValue, 3> clearValues;
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[2].depthStencil = { 1.0f, 0 };

    VkViewport viewport = {};
    viewport.height = (r32)m_pWindow->Height();
    viewport.width = (r32)m_pWindow->Width();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.y = 0.0f;
    viewport.x = 0.0f;

    VkRenderPassBeginInfo renderBegin = { };
    renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBegin.framebuffer = skyFrameBuffer->Handle();
    renderBegin.renderPass = m_pSky->GetSkyboxRenderPass();
    renderBegin.clearValueCount = static_cast<u32>(clearValues.size());
    renderBegin.pClearValues = clearValues.data();
    renderBegin.renderArea.offset = { 0, 0 };
    renderBegin.renderArea.extent = m_pRhi->SwapchainObject()->SwapchainExtent();
    
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
      buf->BindIndexBuffer(ind, 0, VK_INDEX_TYPE_UINT32);
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
  FrameBuffer* gbuffer_FrameBuffer = gResources().GetFrameBuffer(gbuffer_FrameBufferStr);
  GraphicsPipeline* gbuffer_Pipeline = gResources().GetGraphicsPipeline(gbuffer_PipelineStr);
  GraphicsPipeline* gbuffer_StaticPipeline = gResources().GetGraphicsPipeline(gbuffer_StaticPipelineStr);

  if (cmdBuffer && !cmdBuffer->Recording()) {
    cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  }

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  std::array<VkClearValue, 6> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[4].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[5].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo gbuffer_RenderPassInfo = {};
  gbuffer_RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  gbuffer_RenderPassInfo.framebuffer = gbuffer_FrameBuffer->Handle();
  gbuffer_RenderPassInfo.renderPass = gbuffer_FrameBuffer->RenderPass();
  gbuffer_RenderPassInfo.pClearValues = clearValues.data();
  gbuffer_RenderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
  gbuffer_RenderPassInfo.renderArea.extent = m_pRhi->SwapchainObject()->SwapchainExtent();
  gbuffer_RenderPassInfo.renderArea.offset = { 0, 0 };

  VkViewport viewport =  { };
  viewport.height = (r32)m_pWindow->Height();
  viewport.width = (r32)m_pWindow->Width();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  VkDescriptorSet DescriptorSets[6];

  cmdBuffer->Begin(beginInfo);
    cmdBuffer->BeginRenderPass(gbuffer_RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    if (m_pCmdList) {
      for (size_t i = 0; i < m_pCmdList->Size(); ++i) {
        RenderCmd& renderCmd = m_pCmdList->Get(i);
        // Need to notify that this render command does not have a render object.
        if (!renderCmd._pTarget) continue;
        RenderObject* RenderObj = renderCmd._pTarget;
        if (!RenderObj->Renderable) continue;

        b8 Skinned = RenderObj->_pMeshDescId->Skinned();
        GraphicsPipeline* Pipe = Skinned ? gbuffer_Pipeline : gbuffer_StaticPipeline;
        cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipe->Pipeline());
        cmdBuffer->SetViewPorts(0, 1, &viewport);

        DescriptorSets[0] = m_pGlobal->Set()->Handle();
        DescriptorSets[1] = RenderObj->CurrMeshSet()->Handle();
        DescriptorSets[2] = RenderObj->CurrMaterialSet()->Handle();
        DescriptorSets[3] = (Skinned ? RenderObj->CurrBoneSet()->Handle() : nullptr);

        // Bind materials.
        cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, 
          Pipe->Layout(), 
          0,
          (Skinned ? 4 : 3), 
          DescriptorSets, 
          0, 
          nullptr
        );
  
        // Set up the render group.
        for (size_t idx = 0; idx < RenderObj->Size(); ++idx) {
          MeshData* data = (*RenderObj)[idx];

          if (!data) {
            R_DEBUG(rWarning, "Null data in render object group!, skipping...\n");
            continue;
          }

          VertexBuffer* vertexBuffer = data->VertexData();
          IndexBuffer* indexBuffer = data->IndexData();
          VkBuffer vb = vertexBuffer->Handle()->NativeBuffer();

          VkDeviceSize offsets[] = { 0 };
          cmdBuffer->BindVertexBuffers(0, 1, &vb, offsets);

          if (indexBuffer) {
            VkBuffer ib = indexBuffer->Handle()->NativeBuffer();
            cmdBuffer->BindIndexBuffer(ib, 0, VK_INDEX_TYPE_UINT32);
            cmdBuffer->DrawIndexed(indexBuffer->IndexCount(), RenderObj->Instances, 0, 0, 0);
          } else {
            cmdBuffer->Draw(vertexBuffer->VertexCount(), RenderObj->Instances, 0, 0);
          }
        }
      }
    }

    if (m_pDeferredCmdList) {
      for (size_t i = 0; i < m_pDeferredCmdList->Size(); ++i) {

      }
    }
  cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::BuildHDRCmdBuffer(u32 cmdBufferIndex)
{
  if (cmdBufferIndex >= m_HDR._CmdBuffers.size()) {
    R_DEBUG(rError, "Attempted to build HDR cmd buffer. Index out of bounds!\n");
    return;
  }

  CommandBuffer* cmdBuffer = m_HDR._CmdBuffers[cmdBufferIndex];
  if (!cmdBuffer) return;

  VkBuffer vertexBuffer = m_RenderQuad.Quad()->Handle()->NativeBuffer();
  VkBuffer indexBuffer = m_RenderQuad.Indices()->Handle()->NativeBuffer();
  VkDeviceSize offsets[] = { 0 };

  GraphicsPipeline* hdrPipeline = gResources().GetGraphicsPipeline(HDRGammaPipelineStr);
  GraphicsPipeline* Downscale2x = gResources().GetGraphicsPipeline(DownscaleBlurPipeline2xStr);
  GraphicsPipeline* Downscale4x = gResources().GetGraphicsPipeline(DownscaleBlurPipeline4xStr);
  GraphicsPipeline* Downscale8x = gResources().GetGraphicsPipeline(DownscaleBlurPipeline8xStr);
  GraphicsPipeline* Downscale16x = gResources().GetGraphicsPipeline(DownscaleBlurPipeline16xStr);
  GraphicsPipeline* GlowPipeline = gResources().GetGraphicsPipeline(GlowPipelineStr);
  FrameBuffer* hdrFrameBuffer = gResources().GetFrameBuffer(HDRGammaFrameBufferStr);
  FrameBuffer* DownscaleFrameBuffer2x = gResources().GetFrameBuffer(FrameBuffer2xHorizStr);
  FrameBuffer* FB2xFinal = gResources().GetFrameBuffer(FrameBuffer2xFinalStr);
  FrameBuffer* DownscaleFrameBuffer4x = gResources().GetFrameBuffer(FrameBuffer4xStr);
  FrameBuffer* FB4xFinal = gResources().GetFrameBuffer(FrameBuffer4xFinalStr);
  FrameBuffer* DownscaleFrameBuffer8x = gResources().GetFrameBuffer(FrameBuffer8xStr);
  FrameBuffer* FB8xFinal = gResources().GetFrameBuffer(FrameBuffer8xFinalStr);
  FrameBuffer* DownscaleFrameBuffer16x = gResources().GetFrameBuffer(FrameBuffer16xStr);
  FrameBuffer* FB16xFinal = gResources().GetFrameBuffer(FrameBuffer16xFinalStr);
  FrameBuffer* GlowFrameBuffer = gResources().GetFrameBuffer(FrameBufferGlowStr);
  DescriptorSet* hdrSet = gResources().GetDescriptorSet(HDRGammaDescSetStr);
  DescriptorSet* DownscaleSet2x = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet2x);
  DescriptorSet* DownscaleSet4x = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet4x);
  DescriptorSet* DownscaleSet8x = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet8x);
  DescriptorSet* DownscaleSet16x = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet16x);
  DescriptorSet* DownscaleSet2xFinal = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet2xFinalStr);
  DescriptorSet* DownscaleSet4xFinal = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet4xFinalStr);
  DescriptorSet* DownscaleSet8xFinal = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet8xFinalStr);
  DescriptorSet* DownscaleSet16xFinal = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet16xFinalStr);
  DescriptorSet* GlowSet = gResources().GetDescriptorSet(GlowDescriptorSetStr);

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
  renderpassInfo.renderPass = hdrFrameBuffer->RenderPass();
  renderpassInfo.renderArea.extent = m_pRhi->SwapchainObject()->SwapchainExtent();
  renderpassInfo.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass2x =  { };
  DownscalePass2x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass2x.framebuffer = DownscaleFrameBuffer2x->Handle();
  DownscalePass2x.renderPass = DownscaleFrameBuffer2x->RenderPass();
  DownscalePass2x.clearValueCount = 1;
  DownscalePass2x.pClearValues = &clearVal;
  DownscalePass2x.renderArea.extent = { DownscaleFrameBuffer2x->Width(), DownscaleFrameBuffer2x->Height() };
  DownscalePass2x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass4x = { };
  DownscalePass4x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass4x.framebuffer = DownscaleFrameBuffer4x->Handle();
  DownscalePass4x.renderPass = DownscaleFrameBuffer4x->RenderPass();
  DownscalePass4x.clearValueCount = 1;
  DownscalePass4x.pClearValues = &clearVal;
  DownscalePass4x.renderArea.extent = { DownscaleFrameBuffer4x->Width(), DownscaleFrameBuffer4x->Height() };
  DownscalePass4x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass8x = {};
  DownscalePass8x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass8x.framebuffer = DownscaleFrameBuffer8x->Handle();
  DownscalePass8x.renderPass = DownscaleFrameBuffer8x->RenderPass();
  DownscalePass8x.clearValueCount = 1;
  DownscalePass8x.pClearValues = &clearVal;
  DownscalePass8x.renderArea.extent = { DownscaleFrameBuffer8x->Width(), DownscaleFrameBuffer8x->Height() };
  DownscalePass8x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass16x = {};
  DownscalePass16x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass16x.framebuffer = DownscaleFrameBuffer16x->Handle();
  DownscalePass16x.renderPass = DownscaleFrameBuffer16x->RenderPass();
  DownscalePass16x.clearValueCount = 1;
  DownscalePass16x.pClearValues = &clearVal;
  DownscalePass16x.renderArea.extent = { DownscaleFrameBuffer16x->Width(), DownscaleFrameBuffer16x->Height() };
  DownscalePass16x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo GlowPass = {};
  GlowPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  GlowPass.framebuffer = GlowFrameBuffer->Handle();
  GlowPass.renderPass = GlowFrameBuffer->RenderPass();
  GlowPass.clearValueCount = 1;
  GlowPass.pClearValues = &clearVal;
  GlowPass.renderArea.extent = { GlowFrameBuffer->Width(), GlowFrameBuffer->Height() };
  GlowPass.renderArea.offset = { 0, 0 };

  VkViewport viewport = {};
  viewport.height = (r32)m_pWindow->Height();
  viewport.width = (r32)m_pWindow->Width();
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
    viewport.height = (r32)(m_pWindow->Height() >> 1);
    viewport.width =  (r32)(m_pWindow->Width() >> 1);
    cmdBuffer->BeginRenderPass(DownscalePass2x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(r32), &m_Downscale._Horizontal);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 4, sizeof(r32), &m_Downscale._Strength);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 8, sizeof(r32), &m_Downscale._Scale);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();
    DownscalePass2x.framebuffer = FB2xFinal->Handle();
    DownscalePass2x.renderPass = FB2xFinal->RenderPass();
    cmdBuffer->BeginRenderPass(DownscalePass2x, VK_SUBPASS_CONTENTS_INLINE);
      m_Downscale._Horizontal = false;
      DownscaleSetNative = DownscaleSet2xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &m_Downscale._Horizontal);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)(m_pWindow->Height() >> 2);
    viewport.width = (r32)(m_pWindow->Width() >> 2);
    DownscaleSetNative = DownscaleSet4x->Handle();
    i32 _Horizontal = true;
    cmdBuffer->BeginRenderPass(DownscalePass4x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Pipeline());
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    DownscalePass4x.framebuffer = FB4xFinal->Handle();
    DownscalePass4x.renderPass = FB4xFinal->RenderPass();
    cmdBuffer->BeginRenderPass(DownscalePass4x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet4xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)(m_pWindow->Height() >> 3);
    viewport.width = (r32)(m_pWindow->Width() >> 3);
    DownscaleSetNative = DownscaleSet8x->Handle();
    _Horizontal = true;
    cmdBuffer->BeginRenderPass(DownscalePass8x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Pipeline());
      cmdBuffer->PushConstants(Downscale8x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    DownscalePass8x.framebuffer = FB8xFinal->Handle();
    DownscalePass8x.renderPass = FB8xFinal->RenderPass();
    cmdBuffer->BeginRenderPass(DownscalePass8x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet8xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)(m_pWindow->Height() >> 4);
    viewport.width = (r32)(m_pWindow->Width() >> 4);
    DownscaleSetNative = DownscaleSet16x->Handle();
    _Horizontal = true;
    cmdBuffer->BeginRenderPass(DownscalePass16x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->Pipeline());
      cmdBuffer->PushConstants(Downscale16x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    DownscalePass16x.framebuffer = FB16xFinal->Handle();
    DownscalePass16x.renderPass = FB16xFinal->RenderPass();
    cmdBuffer->BeginRenderPass(DownscalePass16x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet16xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale16x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)m_pWindow->Height();
    viewport.width = (r32)m_pWindow->Width();
    VkDescriptorSet GlowDescriptorNative = GlowSet->Handle();
    cmdBuffer->BeginRenderPass(GlowPass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Layout(), 0, 1, &GlowDescriptorNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    VkDescriptorSet dSets[1];
    dSets[0] = hdrSet->Handle();

    cmdBuffer->BeginRenderPass(renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Layout(), 0, 1, dSets, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
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

  GraphicsPipeline* staticPipeline = gResources().GetGraphicsPipeline(ShadowMapPipelineStr);
  GraphicsPipeline* dynamicPipeline = gResources().GetGraphicsPipeline(DynamicShadowMapPipelineStr);  
  DescriptorSet*    lightViewSet = m_pLights->m_pLightViewDescriptorSet;  

  VkCommandBufferBeginInfo begin = { };
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  // No need to record as it is already recording?
  if (cmdBuffer->Recording()) return;

  VkRenderPassBeginInfo renderPass = { };
  renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPass.framebuffer = m_pLights->m_pFrameBuffer->Handle();
  renderPass.renderPass = m_pLights->m_pFrameBuffer->RenderPass();
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

  // Create the shadow rendering pass.
  cmdBuffer->Begin(begin);
    cmdBuffer->BeginRenderPass(renderPass, VK_SUBPASS_CONTENTS_INLINE);
      for (size_t i = 0; i < m_pCmdList->Size(); ++i) {
        RenderCmd& renderCmd = (*m_pCmdList)[i];
        RenderObject* obj = renderCmd._pTarget;
        if (!obj) continue;
        if (!obj->Renderable) continue;
        
        b8 skinned = obj->_pMeshDescId->Skinned();
        VkDescriptorSet descriptorSets[3];
        descriptorSets[0] = obj->CurrMeshSet()->Handle();
        descriptorSets[1] = lightViewSet->Handle();
        descriptorSets[2] = skinned ? obj->CurrBoneSet()->Handle() : VK_NULL_HANDLE;
        GraphicsPipeline* pipeline = skinned ? dynamicPipeline : staticPipeline;
        cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Pipeline());
        cmdBuffer->SetViewPorts(0, 1, &viewport);
        cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Layout(), 0, skinned ? 3 : 2, descriptorSets, 0, nullptr);
        for (size_t idx = 0; idx < obj->Size(); ++idx) {
          MeshData* mesh = (*obj)[idx];
          if (!mesh) return;
          VertexBuffer* vertex = mesh->VertexData();
          IndexBuffer* index = mesh->IndexData();
          VkBuffer buf = vertex->Handle()->NativeBuffer();

          VkDeviceSize offset[] = { 0 };
          cmdBuffer->BindVertexBuffers(0, 1, &buf, offset);
          if (index) {
            VkBuffer ind = index->Handle()->NativeBuffer();
            cmdBuffer->BindIndexBuffer(ind, 0, VK_INDEX_TYPE_UINT32);
            cmdBuffer->DrawIndexed(index->IndexCount(), obj->Instances, 0, 0, 0);
          } else {
            cmdBuffer->Draw(vertex->VertexCount(), obj->Instances, 0, 0);
          }
        }
      }
    cmdBuffer->EndRenderPass();    
  cmdBuffer->End();
}


void Renderer::SetUpPBR()
{
  Sampler* pbr_Sampler = gResources().GetSampler(gbuffer_SamplerStr);

  DescriptorSetLayout* pbr_Layout = gResources().GetDescriptorSetLayout(pbr_DescLayoutStr);
  DescriptorSet* pbr_Set = m_pRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet(pbr_DescSetStr, pbr_Set);

  VkDescriptorImageInfo albedo = {};
  albedo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  albedo.imageView = gResources().GetRenderTexture(gbuffer_AlbedoAttachStr)->View();
  albedo.sampler = pbr_Sampler->Handle();

  VkDescriptorImageInfo normal = {};
  normal.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  normal.imageView = gResources().GetRenderTexture(gbuffer_NormalAttachStr)->View();
  normal.sampler = pbr_Sampler->Handle();

  VkDescriptorImageInfo position = {};
  position.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  position.imageView = gResources().GetRenderTexture(gbuffer_PositionAttachStr)->View();
  position.sampler = pbr_Sampler->Handle();

  VkDescriptorImageInfo roughmetal = {};
  roughmetal.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  roughmetal.imageView = gResources().GetRenderTexture(gbuffer_RoughMetalAttachStr)->View();
  roughmetal.sampler = pbr_Sampler->Handle();

  VkDescriptorImageInfo emission = {};
  emission.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  emission.imageView = gResources().GetRenderTexture(gbuffer_EmissionAttachStr)->View();
  emission.sampler = pbr_Sampler->Handle();

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
  writeInfo[3].pImageInfo = &roughmetal;
  writeInfo[3].pBufferInfo = nullptr;
  writeInfo[3].pTexelBufferView = nullptr;
  writeInfo[3].dstArrayElement = 0;
  writeInfo[3].pNext = nullptr;

  writeInfo[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[4].descriptorCount = 1;
  writeInfo[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[4].dstBinding = 4;
  writeInfo[4].pImageInfo = &emission;
  writeInfo[4].pBufferInfo = nullptr;
  writeInfo[4].pTexelBufferView = nullptr;
  writeInfo[4].dstArrayElement = 0;
  writeInfo[4].pNext = nullptr;
  
  pbr_Set->Allocate(m_pRhi->DescriptorPool(), pbr_Layout);
  pbr_Set->Update(static_cast<u32>(writeInfo.size()), writeInfo.data());

  for (size_t i = 0; i < m_Pbr._CmdBuffers.size(); ++i) {
    m_Pbr._CmdBuffers[i] = m_pRhi->CreateCommandBuffer();
    m_Pbr._CmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  }

  m_Pbr._Sema = m_pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCi = {};
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_Pbr._Sema->Initialize(semaCi);
}


void Renderer::CleanUpPBR()
{
  DescriptorSet* pbr_Set = gResources().UnregisterDescriptorSet(pbr_DescSetStr);
  m_pRhi->FreeDescriptorSet(pbr_Set);

  for (size_t i = 0; i < m_Pbr._CmdBuffers.size(); ++i) {
    m_pRhi->FreeCommandBuffer(m_Pbr._CmdBuffers[i]);
  }

  m_pRhi->FreeVkSemaphore(m_Pbr._Sema);
}


void Renderer::SetUpFinalOutputs()
{
  Texture* pbr_Final = gResources().GetRenderTexture(pbr_FinalTextureStr);
  Texture* hdr_Color = gResources().GetRenderTexture(HDRGammaColorAttachStr);

  Sampler* hdr_Sampler = gResources().GetSampler(HDRGammaSamplerStr);
  Sampler* pbr_Sampler = gResources().GetSampler(gbuffer_SamplerStr);

  DescriptorSetLayout* finalSetLayout = gResources().GetDescriptorSetLayout(FinalDescSetLayoutStr);
  DescriptorSet* offscreenImageDescriptor = m_pRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet(FinalDescSetStr, offscreenImageDescriptor);
  offscreenImageDescriptor->Allocate(m_pRhi->DescriptorPool(), finalSetLayout);

  // TODO(): Final texture must be the hdr post process texture instead!
  VkDescriptorImageInfo renderTextureFinal = {};
  renderTextureFinal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  if (m_HDR._Enabled) {
    renderTextureFinal.sampler = hdr_Sampler->Handle();
    renderTextureFinal.imageView = hdr_Color->View();
  } else {
    renderTextureFinal.sampler = pbr_Sampler->Handle();
    renderTextureFinal.imageView = pbr_Final->View();
  }

  VkDescriptorBufferInfo renderTextureGlobalBuffer = { };
  renderTextureGlobalBuffer.offset = 0;
  renderTextureGlobalBuffer.range = sizeof(GlobalBuffer);
  renderTextureGlobalBuffer.buffer = m_pGlobal->Handle()->NativeBuffer();

  std::array<VkWriteDescriptorSet, 2> writeInfo;
  writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[0].descriptorCount = 1;
  writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[0].dstBinding = 0;
  writeInfo[0].pImageInfo = &renderTextureFinal;
  writeInfo[0].pBufferInfo = nullptr;
  writeInfo[0].pTexelBufferView = nullptr;
  writeInfo[0].dstArrayElement = 0;
  writeInfo[0].pNext = nullptr;

  writeInfo[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[1].descriptorCount = 1;
  writeInfo[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeInfo[1].dstBinding = 1;
  writeInfo[1].pImageInfo = nullptr;
  writeInfo[1].pBufferInfo = &renderTextureGlobalBuffer;
  writeInfo[1].pTexelBufferView = nullptr;
  writeInfo[1].dstArrayElement = 0;
  writeInfo[1].pNext = nullptr;

  offscreenImageDescriptor->Update(static_cast<u32>(writeInfo.size()), writeInfo.data());
}


void Renderer::CheckCmdUpdate()
{
  if (m_NeedsUpdate) {
   if (m_AsyncBuild) {
      BuildAsync();
    } else {
      Build();
    }
  }

  // just in case these were still flagged.
  m_NeedsUpdate = false;
  m_AsyncBuild = false;
}


void Renderer::RenderPrimaryShadows()
{

}


void Renderer::CleanUpFinalOutputs()
{
  DescriptorSet* offscreenDescriptorSet = gResources().UnregisterDescriptorSet(FinalDescSetStr);
  m_pRhi->FreeDescriptorSet(offscreenDescriptorSet);
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


RenderObject* Renderer::CreateRenderObject()
{
  RenderObject* obj = new RenderObject(nullptr, nullptr);
  obj->mRhi = m_pRhi;
  return obj;
}


void Renderer::FreeRenderObject(RenderObject* obj)
{
  if (!obj) return;
  obj->CleanUp();
  delete obj;
}


void Renderer::UpdateMaterials()
{
  // Update global data.
  m_pGlobal->Data()->_EnableAA = m_AntiAliasing;
  Vector4 vec4 = m_pLights->Data()->_PrimaryLight._Direction;
  // Left handed coordinate system, need to flip the z.
  Vector3 sunDir = Vector3(vec4.x, vec4.y, -vec4.z);
  if (m_pGlobal->Data()->_vSunDir != sunDir && sunDir != Vector3()) {
    m_pGlobal->Data()->_vSunDir = sunDir;
    m_pSky->MarkDirty();
  }

  // Update the global descriptor.
  m_pGlobal->Update();

  // Update lights in scene.
  m_pLights->Update();
}


void Renderer::RenderOverlay()
{
  m_pUI->Render();
}


void Renderer::UpdateRendererConfigs(GpuConfigParams* params)
{
  m_pRhi->DeviceWaitIdle();

  if (m_pWindow->Width() <= 0 || m_pWindow <= 0) return;
  VkPresentModeKHR presentMode = m_pRhi->SwapchainObject()->CurrentPresentMode();

  if (params) {
    switch (params->_Buffering) {
      case SINGLE_BUFFER: presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; break;
      case DOUBLE_BUFFER: presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR; break;
      case TRIPLE_BUFFER: presentMode = VK_PRESENT_MODE_MAILBOX_KHR; break;
      default: presentMode = m_pRhi->SwapchainObject()->CurrentPresentMode(); break;
    }

    if (params->_EnableVsync >= 1) {
      presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    // TODO()::
    switch (params->_AA) {
      case AA_None: m_AntiAliasing = false; break;
      case AA_FXAA_2x:
      case AA_FXAA_4x:
      case AA_FXAA_8x:
      default:
        m_AntiAliasing = true; break;
    }

    // TODO():
    switch (params->_Shadows) {
      case SHADOWS_NONE:
      {
        m_pLights->EnablePrimaryShadow(false);
        m_pGlobal->Data()->_EnableShadows = false;
      } break;
      case SHADOWS_LOW:
      case SHADOWS_MEDIUM:
      case SHADOWS_HIGH:
      default:
      {
        m_pLights->EnablePrimaryShadow(true);
        m_pGlobal->Data()->_EnableShadows = true;
      } break;
    }
  }

  if (params && presentMode == m_pRhi->SwapchainObject()->CurrentPresentMode()) {
    // No need to reconstruct the swapchain, since these parameters won't affect
    // the hardcoded pipeline.
    return;
  }

  // Triple buffering atm, we will need to use user params to switch this.
  m_pRhi->ReConfigure(presentMode, m_pWindow->Width(), m_pWindow->Height());

  m_pUI->CleanUp();

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

  m_pUI->Initialize(m_pRhi);
  if (m_pCmdList->Size() > 0) {
    Build();
  }
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
    // signal that no update is required.
    m_NeedsUpdate = false;
    m_AsyncBuild = false;
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


void Renderer::EnableHDR(b8 enable)
{
  if (m_HDR._Enabled != enable) {
    m_HDR._Enabled = enable;
    UpdateRendererConfigs(nullptr);
  }
}
} // Recluse