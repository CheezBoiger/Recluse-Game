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
#include "HDR.hpp"
#include "LightProbe.hpp"
#include "Clusters.hpp"
#include "BakeIBL.hpp"
#include "Particles.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "UIDescriptor.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/ComputePipeline.hpp"
#include "RHI/FrameBuffer.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Shader.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Buffer.hpp"

#include "Core/Core.hpp"
#include "Core/Utility/Profile.hpp"
#include "Filesystem/Filesystem.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"

#include "UI/UI.hpp"

#include <array>

namespace Recluse {

const char* Renderer::appName = "No name";

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
  , m_cpuFence(nullptr)
  , m_pAntiAliasingFXAA(nullptr)
  , m_staticUpdate(false)
  , m_workers(kMaxRendererThreadWorkerCount)
  , m_Minimized(false)
  , m_pGlobalIllumination(nullptr)
  , m_decalEngine(nullptr)
  , m_workGroupSize(0)
  , m_globalLightProbe(nullptr)
  , m_pClusterer(nullptr)
  , m_particleEngine(nullptr)
  , m_usePreRenderSkybox(false)
  , m_pBakeIbl(nullptr)
{
  m_HDR._Enabled = true;
  m_Downscale._Horizontal = 0;
  m_Downscale._Strength = 1.0f;
  m_Downscale._Scale = 1.0f;
  m_skybox._brdfLUT = nullptr;
  m_skybox._envmap = nullptr;
  m_skybox._irradiance = nullptr;
  m_skybox._specular = nullptr;

  m_cmdDeferredList.Resize(1024);
  m_forwardCmdList.Resize(1024);
  m_staticCmdList.Resize(1024);
  m_dynamicCmdList.Resize(1024);
  m_meshDescriptors.Resize(1024);
  m_jointDescriptors.Resize(1024);
  m_materialDescriptors.Resize(1024);

  m_cmdDeferredList.SetSortFunc([&](PrimitiveRenderCmd& cmd1, PrimitiveRenderCmd& cmd2) -> bool {
    
    //if (!cmd1._pTarget->_bRenderable || !cmd2._pTarget->_bRenderable) return false;
    MeshDescriptor* mesh1 = cmd1._pMeshDesc;
    MeshDescriptor* mesh2 = cmd2._pMeshDesc;

    if (!mesh1 || !mesh2) return false;
    Vector3 p1 = cmd1._pPrimitive->_aabb.centroid * mesh1->ObjectData()->_Model;
    Vector3 p2 = cmd2._pPrimitive->_aabb.centroid * mesh2->ObjectData()->_Model;

    Vector4 native_pos = m_pGlobal->Data()->_CameraPos;
    Vector3 cam_pos = Vector3(native_pos.x, native_pos.y, native_pos.z);
    Vector3 v1 = p1 - cam_pos;
    Vector3 v2 = p2 - cam_pos;
    r32 l1 = v1.Length();
    r32 l2 = v2.Length();
    return l1 < l2;
  });

  // Use painter's algorithm in this case for forward, simply because of 
  // transparent objects.
  m_forwardCmdList.SetSortFunc([&](PrimitiveRenderCmd& cmd1, PrimitiveRenderCmd& cmd2) -> bool {
    //if (!cmd1._pTarget->_bRenderable || !cmd2._pTarget->_bRenderable) return false;
    MeshDescriptor* mesh1 = cmd1._pMeshDesc;
    MeshDescriptor* mesh2 = cmd2._pMeshDesc;
    if (!mesh1 || !mesh2) return false;
    Vector3 p1 = cmd1._pPrimitive->_aabb.centroid * mesh1->ObjectData()->_Model;
    Vector3 p2 = cmd1._pPrimitive->_aabb.centroid *  mesh2->ObjectData()->_Model;

    Vector4 native_pos = m_pGlobal->Data()->_CameraPos;
    Vector3 cam_pos = Vector3(native_pos.x, native_pos.y, native_pos.z);
    Vector3 v1 = p1 - cam_pos;
    Vector3 v2 = p2 - cam_pos;

    return v1.Length() > v2.Length();
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

  VulkanRHI::CreateContext(Renderer::appName);
  VulkanRHI::FindPhysicalDevice();
  if (!m_pRhi) m_pRhi = new VulkanRHI();
  SetUpRenderData();
}


void Renderer::OnShutDown()
{
  CleanUpRenderData();
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
  CleanStaticUpdate();
  m_pRhi->Present();
}


void Renderer::WaitForCpuFence()
{
  R_TIMED_PROFILE_RENDERER();

  VkFence fence[] = { m_cpuFence->Handle() };
  m_pRhi->WaitForFences(1, fence, VK_TRUE, UINT64_MAX);
  m_pRhi->ResetFences(1, fence);
}


void Renderer::Render()
{
  R_TIMED_PROFILE_RENDERER();

  if (m_Minimized) {
    // Window was minimized, ignore any cpu draw requests and prevent frame rendering
    // until window is back up.
    ClearCmdLists();
    m_pUI->ClearUiBuffers();
    //WaitForCpuFence();
    return;
  }

  // TODO(): Signal a beginning and end callback or so, when performing 
  // any rendering.
  // Update the scene descriptors before rendering the frame.
  SortCmdLists();

  // Wait for fences before starting next frame.
  //WaitForCpuFence();
  m_pRhi->WaitForFrameInFlightFence();

  // begin frame. This is where we start our render process per frame.
  BeginFrame();
  u32 frameIndex = m_pRhi->CurrentFrame();
  UpdateSceneDescriptors(frameIndex);
  CheckCmdUpdate();

  // TODO(): Need to clean this up.
  VkCommandBuffer offscreen_CmdBuffers[3] = { 
    m_Offscreen._cmdBuffers[frameIndex]->Handle(), 
    nullptr,
    nullptr
  };

  VkSemaphore offscreen_WaitSemas[] = { m_pRhi->CurrentImageAvailableSemaphore() };
  VkSemaphore offscreen_SignalSemas[] = { m_Offscreen._semaphores[frameIndex]->Handle() };
  VkSemaphore shadow_Signals[] = { m_Offscreen._shadowSemaphores[frameIndex]->Handle() };
  VkSemaphore shadow_Waits[] = { m_Offscreen._shadowSemaphores[frameIndex]->Handle(), m_Offscreen._semaphores[frameIndex]->Handle() };

  VkCommandBuffer pbr_CmdBuffers[] = { m_Pbr._CmdBuffers[frameIndex]->Handle() };
  VkSemaphore pbr_SignalSemas[] = { m_Pbr._Semas[frameIndex]->Handle() };
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
  VkSemaphore skybox_SignalSemas[] = { m_SkyboxFinishedSignals[frameIndex]->Handle() };
  VkCommandBuffer skybox_CmdBuffer = m_pSkyboxCmdBuffers[frameIndex]->Handle();
  skyboxSI.commandBufferCount = 1;
  skyboxSI.pCommandBuffers = &skybox_CmdBuffer;
  skyboxSI.pSignalSemaphores = skybox_SignalSemas;
  skyboxSI.pWaitSemaphores = pbr_SignalSemas;

  VkSubmitInfo forwardSi = {};
  VkCommandBuffer forwardBuffers[] = { m_Forward._cmdBuffers[frameIndex]->Handle() };
  VkSemaphore forwardSignalSemas[] = { m_Forward._semaphores[frameIndex]->Handle() };
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
  VkSemaphore hdr_SignalSemas[] = { m_HDR._semaphores[frameIndex]->Handle() };
  VkCommandBuffer hdrCmd = m_HDR._CmdBuffers[frameIndex]->Handle();
  hdrSI.pCommandBuffers = &hdrCmd;
  hdrSI.pSignalSemaphores = hdr_SignalSemas;
  hdrSI.pWaitSemaphores = forwardSignalSemas;

  VkSemaphore* final_WaitSemas = hdr_SignalSemas;
  if (!m_HDR._Enabled) final_WaitSemas = forwardSignalSemas;

  VkSubmitInfo finalSi = { };
  finalSi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkCommandBuffer finalCmdBuffer = { m_pFinalCommandBuffers[frameIndex]->Handle() };
  VkSemaphore finalFinished = { m_pFinalFinishedSemas[frameIndex]->Handle() };
  finalSi.commandBufferCount = 1;
  finalSi.pCommandBuffers = &finalCmdBuffer;
  finalSi.pSignalSemaphores = &finalFinished;
  finalSi.pWaitDstStageMask = waitFlags;
  finalSi.signalSemaphoreCount = 1;
  finalSi.waitSemaphoreCount = 1;
  finalSi.pWaitSemaphores = final_WaitSemas;

  VkSubmitInfo uiSi = { };
  uiSi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkCommandBuffer uiCmdBuffer = { m_pUI->GetCommandBuffer(frameIndex)->Handle() };
  VkSemaphore uiSignalSema = { m_pUI->Signal(frameIndex)->Handle() };
  uiSi.commandBufferCount = 1;
  uiSi.pCommandBuffers = &uiCmdBuffer;
  uiSi.waitSemaphoreCount = 1;
  uiSi.pWaitSemaphores = &finalFinished;
  uiSi.signalSemaphoreCount = 1;
  uiSi.pSignalSemaphores = &uiSignalSema;
  uiSi.pWaitDstStageMask = waitFlags;

  // Spinlock until we know this is finished.
  while (m_Offscreen._cmdBuffers[frameIndex]->Recording()) {}

  // Render shadow map here. Primary shadow map is our concern.
  if (m_pLights->PrimaryShadowEnabled() || StaticNeedsUpdate()) {
    R_DEBUG(rNotify, "Shadow.\n");
    u32 graphicsQueueCount = m_pRhi->GraphicsQueueCount();
    if (graphicsQueueCount > 1) {

      VkSubmitInfo shadowSI = {};
      shadowSI.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      shadowSI.commandBufferCount = 1;
      VkCommandBuffer shadowCmdBuffer = m_Offscreen._shadowCmdBuffers[frameIndex]->Handle();
      shadowSI.pCommandBuffers = &shadowCmdBuffer;
      shadowSI.pSignalSemaphores = shadow_Signals;
      shadowSI.pWaitDstStageMask = waitFlags;
      shadowSI.pWaitSemaphores = VK_NULL_HANDLE;
      shadowSI.signalSemaphoreCount = 1;
      shadowSI.waitSemaphoreCount = 0;
        
      pbrSi.waitSemaphoreCount = 2;
      pbrSi.pWaitSemaphores = shadow_Waits;
      m_pRhi->GraphicsSubmit(1, 1, &shadowSI);
    } else {
      offscreen_CmdBuffers[offscreenSI.commandBufferCount] = m_Offscreen._shadowCmdBuffers[frameIndex]->Handle();
      offscreenSI.commandBufferCount += 1; // Add shadow buffer to render.
    }
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

  //
  // TODO(): Add antialiasing here.
  // 

  // High Dynamic Range and Gamma Pass. Post process after rendering. This will include
  // Bloom, AA, other effects.
  if (m_HDR._Enabled) { m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 1, &hdrSI); }

  // Final render after post process.
  m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 1, &finalSi);

  // Before calling this cmd buffer, we want to submit our offscreen buffer first, then
  // sent our signal to our swapchain cmd buffers.
    
  // Render the Overlay.
  m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 1, &uiSi);

  // Signal graphics finished on the final output.
  VkSemaphore signal = m_pRhi->CurrentGraphicsFinishedSemaphore();
  VkFence inflight = m_pRhi->CurrentInFlightFence();
  m_pRhi->SubmitCurrSwapchainCmdBuffer(1, &uiSignalSema, 1, &signal, inflight); // cpuFence will need to wait until overlay is finished.
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


  m_pAntiAliasingFXAA->CleanUp(m_pRhi);
  delete m_pAntiAliasingFXAA;
  m_pAntiAliasingFXAA = nullptr;

  m_pBakeIbl->CleanUp(m_pRhi);
  delete m_pBakeIbl;
  m_pBakeIbl = nullptr;

  m_pHDR->CleanUp(m_pRhi);
  delete m_pHDR;
  m_pHDR = nullptr;

  // We probably want to use smart ptrs...
  m_pGlobal->CleanUp(m_pRhi);
  delete m_pGlobal;
  m_pGlobal = nullptr;

  m_pLights->CleanUp(m_pRhi);
  delete m_pLights;
  m_pLights = nullptr;
  
  CleanUpSkybox(false);
  m_pSky->CleanUp();
  delete m_pSky;
  m_pSky = nullptr;

  if (m_pUI) {
    m_pUI->CleanUp();
    delete m_pUI;
    m_pUI = nullptr;
  }

  if (m_particleEngine) {
    m_particleEngine->CleanUp(m_pRhi);
    delete m_particleEngine;
    m_particleEngine = nullptr;
  }

  CleanUpGlobalIlluminationBuffer();
  delete m_pGlobalIllumination;
  m_pGlobalIllumination = nullptr;

  m_RenderQuad.CleanUp(m_pRhi);
  CleanUpForwardPBR();
  CleanUpPBR();
  CleanUpHDR(true);
  CleanUpDownscale(true);
  CleanUpOffscreen();
  CleanUpFinalOutputs();
  CleanUpDescriptorSetLayouts();
  CleanUpGraphicsPipelines();
  ShadowMapSystem::CleanUpShadowPipelines(m_pRhi);
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
  gMat->Initialize(m_pRhi);
  gMat->Data()->_ScreenSize[0] = window->Width();
  gMat->Data()->_ScreenSize[1] = window->Height();
  for (u32 i = 0; i < m_pRhi->BufferingCount(); ++i) {
    gMat->Update(m_pRhi, i);
  }
  m_pGlobal = gMat;

  m_pSky = new SkyRenderer();
  m_pSky->Initialize();
  m_pSky->MarkDirty();

  m_pHDR = new HDR();
  m_pHDR->Initialize(m_pRhi);
  m_pHDR->UpdateToGPU(m_pRhi);

  SetUpSkybox(false);
  SetUpGraphicsPipelines();

  // Dependency on shadow map pipeline initialization.
  ShadowMapSystem::InitializeShadowPipelines(m_pRhi);
  m_pLights = new LightDescriptor();
  m_pLights->Initialize(m_pRhi, params->_Shadows, params->_EnableSoftShadows);
  for (u32 i = 0; i < m_pRhi->BufferingCount(); ++i) {
    m_pLights->Update(m_pRhi, m_pGlobal->Data(), i);
  }

  UpdateRuntimeConfigs(params);
  m_pAntiAliasingFXAA = new AntiAliasingFXAA();
  m_pAntiAliasingFXAA->Initialize(m_pRhi, m_pGlobal);

  SetUpFinalOutputs();
  SetUpOffscreen();
  SetUpDownscale(true);
  SetUpHDR(true);
  SetUpPBR();
  SetUpForwardPBR();

  m_pBakeIbl = new BakeIBL();
  m_pBakeIbl->Initialize(m_pRhi);

  m_particleEngine = new ParticleEngine();
  m_particleEngine->Initialize(m_pRhi);

  {
    u32 vendorId = m_pRhi->VendorID();
    switch (vendorId) {
      case AMD_VENDOR_ID: m_workGroupSize = AMD_WAVEFRONT_SIZE; break;
      case NVIDIA_VENDOR_ID: m_workGroupSize = NVIDIA_WARP_SIZE; break;
      case INTEL_VENDOR_ID:
      default:
        m_workGroupSize = INTEL_WORKGROUP_SIZE;
        break;
    } 
    R_DEBUG(rDebug, "Workgroup size is " << m_workGroupSize << '\n');
  }

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

  if (!m_pUI) {
    m_pUI = new UIOverlay();
    m_pUI->Initialize(m_pRhi);
  }

  m_pGlobalIllumination = new GlobalIllumination();

  SetUpGlobalIlluminationBuffer();

  UpdateGlobalIlluminationBuffer();

  m_Initialized = true;
  return true;
}


void Renderer::SetUpDescriptorSetLayouts()
{
  // Light space.
  {
    std::array<VkDescriptorSetLayoutBinding, 2> LightViewBindings;
    LightViewBindings[0].pImmutableSamplers = nullptr;
    LightViewBindings[0].binding = 0;
    LightViewBindings[0].descriptorCount = 1;
    LightViewBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    LightViewBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT 
      | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    LightViewBindings[1].binding = 1;
    LightViewBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    LightViewBindings[1].descriptorCount = 1;
    LightViewBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    LightViewBindings[1].pImmutableSamplers = nullptr;

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
    GlobalBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT 
      | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
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
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;  

    // Normal
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].pImmutableSamplers = nullptr;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;  

    // Position
    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].pImmutableSamplers = nullptr;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    // Emission-Roughness-Metallic
    bindings[3].binding = 3;
    bindings[3].descriptorCount = 1;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].pImmutableSamplers = nullptr;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    // Depth
    bindings[4].binding = 4;
    bindings[4].descriptorCount = 1;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].pImmutableSamplers = nullptr;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT; 

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
    globalIllumination_DescNoLR = m_pRhi->CreateDescriptorSetLayout();
    globalIllumination_DescLR = m_pRhi->CreateDescriptorSetLayout();
    
    std::array<VkDescriptorSetLayoutBinding, 6> globalIllum;
    // Global IrrMap.
    globalIllum[0].binding = 0;
    globalIllum[0].descriptorCount = 1;
    globalIllum[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    globalIllum[0].pImmutableSamplers = nullptr;
    globalIllum[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    // Global EnvMap.
    globalIllum[1].binding = 1;
    globalIllum[1].descriptorCount = 1;
    globalIllum[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globalIllum[1].pImmutableSamplers = nullptr;
    globalIllum[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT; 
    // Global BRDF lookup table
    globalIllum[2].binding = 2;
    globalIllum[2].descriptorCount = 1;
    globalIllum[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globalIllum[2].pImmutableSamplers = nullptr;
    globalIllum[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    // Irradiance Map array.
    globalIllum[3].binding = 3;
    globalIllum[3].descriptorCount = 1;
    globalIllum[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    globalIllum[3].pImmutableSamplers = nullptr;
    globalIllum[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    // Radiance (Enviroment Map) array.
    globalIllum[4].binding = 4;
    globalIllum[4].descriptorCount = 1;
    globalIllum[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globalIllum[4].pImmutableSamplers = nullptr;
    globalIllum[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    // BRDF lookup table
    globalIllum[5].binding = 5;
    globalIllum[5].descriptorCount = 1;
    globalIllum[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    globalIllum[5].pImmutableSamplers = nullptr;
    globalIllum[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.flags = 0;
    info.bindingCount = static_cast<u32>(globalIllum.size());
    info.pBindings = globalIllum.data();
    info.pNext = nullptr;
    globalIllumination_DescLR->Initialize(info);
    info.bindingCount = static_cast<u32>(globalIllum.size() - 3);
    globalIllumination_DescNoLR->Initialize(info);
  }

  // Compute PBR textures.
  {
    pbr_compDescLayout = m_pRhi->CreateDescriptorSetLayout();
    std::array<VkDescriptorSetLayoutBinding, 2> bindings;
    bindings[0] = { };
    bindings[1] = { };

    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo info = { };
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<u32>(bindings.size());
    info.pBindings = bindings.data();

    pbr_compDescLayout->Initialize(info);
  }

  // Light layout.
  {
    std::array<VkDescriptorSetLayoutBinding, 1> LightBindings;
    LightBindings[0].binding = 0;
    LightBindings[0].descriptorCount = 1;
    LightBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    LightBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    LightBindings[0].pImmutableSamplers = nullptr;

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
  std::array<VkDescriptorSetLayoutBinding, 2> hdrBindings;
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
  m_pRhi->FreeDescriptorSetLayout(pbr_compDescLayout);
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

  std::array<VkAttachmentReference, 6> attachmentColors;
  VkAttachmentReference attachmentDepthRef = { 4u, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
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

  attachmentColors[5].attachment = 5;
  attachmentColors[5].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = { };
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<u32>(attachmentColors.size() - 2);
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
    std::array<VkAttachmentDescription, 7> pbrAttachmentDescriptions;
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
    pbrSubpass.colorAttachmentCount = static_cast<u32>(pbrAttachmentDescriptions.size() - 5);
    pbrSubpass.pColorAttachments = attachmentColors.data();
    attachmentDepthRef.attachment = 2;
    pbrSubpass.pDepthStencilAttachment = &attachmentDepthRef;

    VkRenderPassCreateInfo pbrRenderpassCI = CreateRenderPassInfo(
      static_cast<u32>(pbrAttachmentDescriptions.size() - 4),
      pbrAttachmentDescriptions.data(),
      2,
      dependencies,
      1,
      &pbrSubpass
    );

    std::array<VkImageView, 7> pbrAttachments;
    pbrAttachments[0] = pbr_Final->View();
    pbrAttachments[1] = pbr_Bright->View();
    pbrAttachments[2] = gbuffer_Depth->View();

    VkFramebufferCreateInfo pbrFramebufferCI = CreateFrameBufferInfo(
      windowExtent.width,
      windowExtent.height,
      nullptr, // Finalize() call handles this for us.
      static_cast<u32>(pbrAttachments.size() - 4),
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
      gbuffer_Albedo->Format(),
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Albedo->Samples()
    );

    pbrAttachmentDescriptions[3] = CreateAttachmentDescription(
      gbuffer_Normal->Format(),
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Normal->Samples()
    );

    pbrAttachmentDescriptions[4] = CreateAttachmentDescription(
      gbuffer_Position->Format(),
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Position->Samples()
    );

    pbrAttachmentDescriptions[5] = CreateAttachmentDescription(
      gbuffer_Emission->Format(),
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Emission->Samples()
    );

    pbrAttachmentDescriptions[6] = CreateAttachmentDescription(
      gbuffer_Depth->Format(),
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Depth->Samples()
    );

    pbrAttachments[0] = pbr_Final->View();
    pbrAttachments[1] = pbr_Bright->View();
    pbrAttachments[2] = gbuffer_Albedo->View();
    pbrAttachments[3] = gbuffer_Normal->View();
    pbrAttachments[4] = gbuffer_Position->View();
    pbrAttachments[5] = gbuffer_Emission->View();
    pbrAttachments[6] = gbuffer_Depth->View();

    pbrSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    pbrSubpass.colorAttachmentCount = static_cast<u32>(pbrAttachmentDescriptions.size() - 1);
    pbrSubpass.pColorAttachments = attachmentColors.data();
    attachmentDepthRef.attachment = 6;
    pbrSubpass.pDepthStencilAttachment = &attachmentDepthRef;

    pbrRenderpassCI = CreateRenderPassInfo(
      static_cast<u32>(pbrAttachmentDescriptions.size()),
      pbrAttachmentDescriptions.data(),
      2,
      dependencies,
      1,
      &pbrSubpass
    );

    pbrFramebufferCI = CreateFrameBufferInfo(
      windowExtent.width,
      windowExtent.height,
      nullptr, // Finalize() call handles this for us.
      static_cast<u32>(pbrAttachments.size()),
      pbrAttachments.data(),
      1
    );

    pbr_forwardRenderPass = m_pRhi->CreateRenderPass();
    pbr_forwardFrameBuffer = m_pRhi->CreateFrameBuffer();
    pbr_forwardRenderPass->Initialize(pbrRenderpassCI);
    pbr_forwardFrameBuffer->Finalize(pbrFramebufferCI, pbr_forwardRenderPass);
    
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
  depthStencilCI.depthBoundsTestEnable = m_pRhi->DepthBoundsAllowed();
  depthStencilCI.minDepthBounds = 0.0f;
  depthStencilCI.maxDepthBounds = 1.0f;
  depthStencilCI.stencilTestEnable = VK_FALSE;
  depthStencilCI.back.compareMask = 0xff;
  depthStencilCI.back.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilCI.back.passOp = VK_STENCIL_OP_REPLACE;
  depthStencilCI.back.failOp = VK_STENCIL_OP_ZERO;
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
    
  RendererPass::SetUpForwardPhysicallyBasedPass(RHI(), GraphicsPipelineInfo);

  RendererPass::SetUpGBufferPass(RHI(), GraphicsPipelineInfo);
  RendererPass::SetUpSkyboxPass(RHI(), GraphicsPipelineInfo);

  // Set to quad rendering format.
  colorBlendCI.logicOpEnable = VK_FALSE;
  depthStencilCI.depthTestEnable = VK_FALSE;
  depthStencilCI.depthBoundsTestEnable = VK_FALSE;
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
  RendererPass::SetUpHDRGammaPass(RHI(), GraphicsPipelineInfo, m_pHDR);

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
  m_pRhi->FreeGraphicsPipeline(gbuffer_staticMorphTargetPipeline);
  m_pRhi->FreeGraphicsPipeline(gbuffer_morphTargetPipeline);
  m_pRhi->FreeGraphicsPipeline(pbr_forwardPipeline_LR);
  m_pRhi->FreeGraphicsPipeline(pbr_staticForwardPipeline_LR);
  m_pRhi->FreeGraphicsPipeline(pbr_forwardPipeline_NoLR);
  m_pRhi->FreeGraphicsPipeline(pbr_staticForwardPipeline_NoLR);
  m_pRhi->FreeGraphicsPipeline(transparent_staticShadowPipe);
  m_pRhi->FreeGraphicsPipeline(transparent_dynamicShadowPipe);
  m_pRhi->FreeGraphicsPipeline(transparent_colorFilterPipe);
  m_pRhi->FreeGraphicsPipeline(pbr_forwardPipelineMorphTargets_LR);
  m_pRhi->FreeGraphicsPipeline(pbr_forwardPipelineMorphTargets_NoLR);
  m_pRhi->FreeGraphicsPipeline(pbr_staticForwardPipelineMorphTargets_LR);
  m_pRhi->FreeGraphicsPipeline(pbr_staticForwardPipelineMorphTargets_NoLR);
  m_pRhi->FreeGraphicsPipeline(pbr_static_LR_Debug);
  m_pRhi->FreeGraphicsPipeline(pbr_static_NoLR_Debug);
  m_pRhi->FreeGraphicsPipeline(pbr_static_mt_LR_Debug);
  m_pRhi->FreeGraphicsPipeline(pbr_static_mt_NoLR_Debug);
  m_pRhi->FreeGraphicsPipeline(pbr_dynamic_LR_Debug);
  m_pRhi->FreeGraphicsPipeline(pbr_dynamic_LR_mt_Debug);
  m_pRhi->FreeGraphicsPipeline(pbr_dynamic_NoLR_Debug);
  m_pRhi->FreeGraphicsPipeline(pbr_dynamic_NoLR_mt_Debug);

  GraphicsPipeline* QuadPipeline = final_PipelineKey;
  m_pRhi->FreeGraphicsPipeline(QuadPipeline);

  m_pRhi->FreeGraphicsPipeline(output_pipelineKey);
  m_pRhi->FreeComputePipeline(pbr_computePipeline_NoLR);
  m_pRhi->FreeComputePipeline(pbr_computePipeline_LR);

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
  m_pRhi->FreeFrameBuffer(pbr_forwardFrameBuffer);
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

  RDEBUG_SET_VULKAN_NAME(gbuffer_Albedo, "Albedo");
  RDEBUG_SET_VULKAN_NAME(gbuffer_Normal, "Normal");
  RDEBUG_SET_VULKAN_NAME(gbuffer_roughMetalSpec, "RoughMetal");
  RDEBUG_SET_VULKAN_NAME(gbuffer_Depth, "Depth");
  RDEBUG_SET_VULKAN_NAME(gbuffer_Emission, "Emissive");

  RDEBUG_SET_VULKAN_NAME(pbr_Final, "PBR Lighting");
  RDEBUG_SET_VULKAN_NAME(pbr_Bright, "Bright");
  RDEBUG_SET_VULKAN_NAME(GlowTarget, "Glow");

  Texture* hdr_Texture = m_pRhi->CreateTexture();
  Sampler* hdr_Sampler = m_pRhi->CreateSampler();
  RDEBUG_SET_VULKAN_NAME(hdr_Texture, "HDR");
  
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

  cImageInfo.format = GBUFFER_ADDITIONAL_INFO_FORMAT;
  cViewInfo.format =  GBUFFER_ADDITIONAL_INFO_FORMAT;
  gbuffer_Emission->Initialize(cImageInfo, cViewInfo);

  cImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  pbr_Final->Initialize(cImageInfo, cViewInfo);
  final_renderTexture->Initialize(cImageInfo, cViewInfo);
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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

  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  pbr_Bright->Initialize(cImageInfo, cViewInfo);
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  GlowTarget->Initialize(cImageInfo, cViewInfo);

  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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
  samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  samplerCI.maxLod = 16.0f;
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
    dImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    dImageInfo.imageType = VK_IMAGE_TYPE_2D;
    dImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    dImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dImageInfo.mipLevels = 1;
    dImageInfo.extent.depth = 1;
    dImageInfo.arrayLayers = 1;
    dImageInfo.extent.width = 1;
    dImageInfo.extent.height = 1;
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
    RDEBUG_SET_VULKAN_NAME(defaultTexture, "Default Texture");
    defaultTexture->Initialize(dImageInfo, dViewInfo);
    DefaultTextureKey = defaultTexture;

    {
      Texture2D tex2d;
      tex2d.mRhi = m_pRhi;
      tex2d.texture = defaultTexture;
      tex2d.m_bGenMips = false;
      Image img; img._data = new u8[4]; img._height = 1; img._width = 1; img._memorySize = 4;
      tex2d.Update(img);
      delete img._data;
    }

    if (!DefaultTexture2DArrayView) {
      dViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
      dViewInfo.image = defaultTexture->Image();
      vkCreateImageView(m_pRhi->LogicDevice()->Native(), &dViewInfo, nullptr, &DefaultTexture2DArrayView);
    }
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

    if (DefaultTexture2DArrayView) {
      vkDestroyImageView(m_pRhi->LogicDevice()->Native(), DefaultTexture2DArrayView, nullptr);
      DefaultTexture2DArrayView = VK_NULL_HANDLE;
    }
  }
}


void Renderer::GeneratePbrCmds(CommandBuffer* cmdBuffer, u32 frameIndex) 
{
  GraphicsPipeline* pPipeline = nullptr;
  ComputePipeline* pCompPipeline = nullptr;
  pPipeline = pbr_Pipeline_NoLR;
  pCompPipeline = pbr_computePipeline_NoLR;

  if (m_currentGraphicsConfigs._EnableLocalReflections) {
    pPipeline = pbr_Pipeline_LR;
    pCompPipeline = pbr_computePipeline_LR;
  }

  FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;

  VkExtent2D windowExtent = { 
    (u32)m_pGlobal->Data()->_ScreenSize[0], 
    (u32)m_pGlobal->Data()->_ScreenSize[1] 
  };
  VkViewport viewport = {};
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

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
#if !COMPUTE_PBR
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
#else
    std::array<VkImageMemoryBarrier, 2> imageMemBarriers;
    imageMemBarriers[0] = { };
    imageMemBarriers[1] = { };

    VkImageSubresourceRange subrange = { };
    subrange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subrange.baseArrayLayer = 0;
    subrange.baseMipLevel = 0;
    subrange.layerCount = 1;
    subrange.levelCount = 1;

    imageMemBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemBarriers[0].subresourceRange = subrange;
    imageMemBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemBarriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemBarriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageMemBarriers[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    imageMemBarriers[0].image = pbr_FinalTextureKey->Image();

    imageMemBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemBarriers[1].subresourceRange = subrange;
    imageMemBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemBarriers[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemBarriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageMemBarriers[1].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    imageMemBarriers[1].image = pbr_BrightTextureKey->Image();

    cmdBuffer->PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0, 0, nullptr, 0, nullptr, static_cast<u32>(imageMemBarriers.size()), imageMemBarriers.data());
    ShadowMapSystem& shadow = m_pLights->PrimaryShadowMapSystem(); 
    VkDescriptorSet compSets[] = { 
      m_pGlobal->Set(frameIndex)->Handle(),
      pbr_DescSetKey->Handle(),
      m_pLights->Set()->Handle(),
      shadow.ShadowMapViewDescriptor(frameIndex)->Handle(),
      shadow.StaticShadowMapViewDescriptor(frameIndex)->Handle(),
      m_pGlobalIllumination->GetDescriptorSet()->Handle(),
      pbr_compSet->Handle()
    };

    VkImageSubresourceRange range = { };
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseArrayLayer = 0;
    range.baseMipLevel = 0;
    range.layerCount = 1;
    range.levelCount = 1;
    VkClearColorValue clColor = { 0.0f, 0.0f, 0.0f, 0.0f };

    cmdBuffer->ClearColorImage(pbr_FinalTextureKey->Image(), VK_IMAGE_LAYOUT_GENERAL, &clColor, 1, &range);
    cmdBuffer->ClearColorImage(pbr_BrightTextureKey->Image(), VK_IMAGE_LAYOUT_GENERAL, &clColor, 1, &range);

    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, pCompPipeline->Pipeline());
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, pCompPipeline->Layout(), 
      0, 7, compSets, 0, nullptr);
    cmdBuffer->Dispatch((windowExtent.width / m_workGroupSize) + 1, (windowExtent.height / m_workGroupSize) + 1, 1);

    imageMemBarriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageMemBarriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    imageMemBarriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    imageMemBarriers[1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemBarriers[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageMemBarriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    imageMemBarriers[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    cmdBuffer->PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0, 0, nullptr, 0, nullptr, static_cast<u32>(imageMemBarriers.size()), imageMemBarriers.data());
#endif
}


void Renderer::SetUpOffscreen()
{
  CleanUpOffscreen();

  m_Offscreen._cmdBuffers.resize(m_pRhi->BufferingCount());
  m_Offscreen._shadowCmdBuffers.resize(m_pRhi->BufferingCount());

  m_Offscreen._semaphores.resize(m_pRhi->BufferingCount());
  m_Offscreen._shadowSemaphores.resize(m_pRhi->BufferingCount());

  VkSemaphoreCreateInfo semaCI = { };
  semaCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


  for (size_t i = 0; i < m_Offscreen._cmdBuffers.size(); ++i) {
    m_Offscreen._cmdBuffers[i] = m_pRhi->CreateCommandBuffer();
    m_Offscreen._cmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    m_Offscreen._shadowCmdBuffers[i] = m_pRhi->CreateCommandBuffer();
    m_Offscreen._shadowCmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(1), VK_COMMAND_BUFFER_LEVEL_PRIMARY); 
    m_Offscreen._semaphores[i] = m_pRhi->CreateVkSemaphore();
    m_Offscreen._shadowSemaphores[i] = m_pRhi->CreateVkSemaphore();
    m_Offscreen._semaphores[i]->Initialize(semaCI);
    m_Offscreen._shadowSemaphores[i]->Initialize(semaCI);
  }
}


void Renderer::CleanUpOffscreen()
{
  for (size_t i = 0; i < m_Offscreen._cmdBuffers.size(); ++i) {
    m_pRhi->FreeCommandBuffer(m_Offscreen._cmdBuffers[i]);
    m_pRhi->FreeCommandBuffer(m_Offscreen._shadowCmdBuffers[i]);
    m_pRhi->FreeVkSemaphore(m_Offscreen._semaphores[i]);
    m_pRhi->FreeVkSemaphore(m_Offscreen._shadowSemaphores[i]);
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
  Img.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  
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
  CleanUpHDR(fullSetUp);
  if (fullSetUp) {
    m_HDR._CmdBuffers.resize(m_pRhi->BufferingCount());
    for (u32 i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
      m_HDR._CmdBuffers[i] = m_pRhi->CreateCommandBuffer();
      m_HDR._CmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
  }

  VkSemaphoreCreateInfo semaCi = {};
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_HDR._semaphores.resize(m_pRhi->BufferingCount());
  for (u32 i = 0; i < m_HDR._semaphores.size(); ++i) {
    m_HDR._semaphores[i] = m_pRhi->CreateVkSemaphore();
    m_HDR._semaphores[i]->Initialize(semaCi);
  }

  switch (m_currentGraphicsConfigs._AA) {
  case AA_FXAA_2x: m_pAntiAliasingFXAA->UpdateSets(m_pRhi, m_pGlobal);
  }

  DescriptorSet* hdrSet = m_pRhi->CreateDescriptorSet();
  hdr_gamma_descSetKey = hdrSet;
  std::array<VkWriteDescriptorSet, 2> hdrWrites;

  VkDescriptorImageInfo pbrImageInfo = { };
  switch (m_currentGraphicsConfigs._AA) {
    case AA_FXAA_2x:
    {
      Texture* texture = m_pAntiAliasingFXAA->GetOutput();
      Sampler* sampler = m_pAntiAliasingFXAA->GetOutputSampler();
      pbrImageInfo.sampler = sampler->Handle();
      pbrImageInfo.imageView = texture->View();
      pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } break;
    case AA_None:
    default:  
    {
      pbrImageInfo.sampler = gbuffer_SamplerKey->Handle();
      pbrImageInfo.imageView = pbr_FinalTextureKey->View();
      pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  }
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

  // Allocate and update the hdr buffer.
  hdrSet->Allocate(m_pRhi->DescriptorPool(), hdr_gamma_descSetLayoutKey);
  hdrSet->Update(static_cast<u32>(hdrWrites.size()), hdrWrites.data());
}


void Renderer::CleanUpHDR(b32 fullCleanUp)
{
  if (fullCleanUp) {
    for (u32 i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
      m_pRhi->FreeCommandBuffer(m_HDR._CmdBuffers[i]);
      m_HDR._CmdBuffers[i] = nullptr;
    }
  }

  if (hdr_gamma_descSetKey) {
    m_pRhi->FreeDescriptorSet(hdr_gamma_descSetKey);
    hdr_gamma_descSetKey = nullptr;
  }

  for (u32 i = 0; i < m_HDR._semaphores.size(); ++i) {
    m_pRhi->FreeVkSemaphore(m_HDR._semaphores[i]);
    m_HDR._semaphores[i] = nullptr;
  }
}


void Renderer::BuildOffScreenCmdList()
{
  R_ASSERT(m_pRhi->BufferingCount() == m_Offscreen._cmdBuffers.size(), "Attempted to build offscreen cmd buffer. Index out of bounds!\n");
  for (u32 i = 0; i < m_Offscreen._cmdBuffers.size(); ++i) {
    CommandBuffer* cmdBuf = m_Offscreen._cmdBuffers[i];
    R_ASSERT(cmdBuf, "Offscreen cmd buffer is null.");
    cmdBuf->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdBuf->Begin(beginInfo);
    GenerateOffScreenCmds(cmdBuf, i);
    cmdBuf->End();
  }
}


void Renderer::BuildHDRCmdList()
{
  for (u32 i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_HDR._CmdBuffers[i];
    R_ASSERT(pCmdBuffer, "HDR buffer is null");
    pCmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo cmdBi = {};
    cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    pCmdBuffer->Begin(cmdBi);
    GenerateHDRCmds(pCmdBuffer, i);
    pCmdBuffer->End();
  }
}


void Renderer::BuildShadowCmdList()
{
  u32 frameIndex = m_pRhi->CurrentFrame();
  CommandBuffer* shadowBuf = m_Offscreen._shadowCmdBuffers[frameIndex];
  R_ASSERT(shadowBuf, "Shadow Buffer is null.");
  shadowBuf->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  VkCommandBufferBeginInfo begin = {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  shadowBuf->Begin(begin);
  GenerateShadowCmds(shadowBuf, frameIndex);
  shadowBuf->End();
}


void Renderer::BuildPbrCmdLists()
{
  for (u32 i = 0; i < m_Pbr._CmdBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_Pbr._CmdBuffers[i];
    R_ASSERT(pCmdBuffer, "PBR command buffer is null.");
    pCmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    pCmdBuffer->Begin(beginInfo);
    GeneratePbrCmds(pCmdBuffer, i);
    pCmdBuffer->End();
  }
}


void Renderer::BuildSkyboxCmdLists()
{
  for (u32 i = 0; i < m_pSkyboxCmdBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_pSkyboxCmdBuffers[i];
    R_ASSERT(pCmdBuffer, "Skybox buffer is null");
    pCmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    pCmdBuffer->Begin(beginInfo);
    GenerateSkyboxCmds(pCmdBuffer, i);
    pCmdBuffer->End();
  }
}


void Renderer::BuildFinalCmdLists()
{
  for (u32 i = 0; i < m_pFinalCommandBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_pFinalCommandBuffers[i];
    R_ASSERT(pCmdBuffer, "Final Command buffer is null.");
    pCmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo cmdBi = {};
    cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    pCmdBuffer->Begin(cmdBi);
    GenerateFinalCmds(pCmdBuffer);
    pCmdBuffer->End();
  }
}


void Renderer::Build()
{
  m_pRhi->WaitAllGraphicsQueues();
  
  BuildOffScreenCmdList();
  BuildHDRCmdList();
  BuildShadowCmdList();
  BuildPbrCmdLists();
  BuildSkyboxCmdLists();
  BuildFinalCmdLists();
  m_pRhi->RebuildCommandBuffers(m_pRhi->CurrentSwapchainCmdBufferSet());
}


void Renderer::UpdateSkyboxCubeMap()
{
  DescriptorSet* skyboxSet = skybox_descriptorSetKey;
  Texture* cubemap = m_pSky->GetCubeMap();
  if (m_usePreRenderSkybox) {
    cubemap = m_preRenderSkybox->Handle(); 
  }
  VkDescriptorImageInfo image = {};
  image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image.imageView = cubemap->View();
  image.sampler = m_pSky->GetSampler()->Handle();

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstArrayElement = 0;
  write.dstBinding = 0;
  write.pImageInfo = &image;

  skyboxSet->Update(1, &write);
}


void Renderer::SetUpSkybox(b32 justSemaphores)
{
  CleanUpSkybox(justSemaphores);
  if (!justSemaphores) {
    DescriptorSet* skyboxSet = m_pRhi->CreateDescriptorSet();
    skybox_descriptorSetKey = skyboxSet;
    DescriptorSetLayout* layout = skybox_setLayoutKey;

    skyboxSet->Allocate(m_pRhi->DescriptorPool(), layout);

    UpdateSkyboxCubeMap();
  }

  // Create skybox Commandbuffer.
  m_pSkyboxCmdBuffers.resize(m_pRhi->BufferingCount());
  for (u32 i = 0; i < m_pSkyboxCmdBuffers.size(); ++i) {
    m_pSkyboxCmdBuffers[i] = m_pRhi->CreateCommandBuffer();
    m_pSkyboxCmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  }
  m_SkyboxFinishedSignals.resize(m_pRhi->BufferingCount());
  VkSemaphoreCreateInfo sema = {};
  sema.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (u32 i = 0; i < m_SkyboxFinishedSignals.size(); ++i) {
    m_SkyboxFinishedSignals[i] = m_pRhi->CreateVkSemaphore();
    m_SkyboxFinishedSignals[i]->Initialize(sema);
  }
}


void Renderer::UsePreRenderSkyboxMap(b32 enable)
{
  m_usePreRenderSkybox = enable;
  m_pRhi->GraphicsWaitIdle(0);
  UpdateSkyboxCubeMap();
  UpdateGlobalIlluminationBuffer();

  BuildSkyboxCmdLists();
  BuildPbrCmdLists();
}


void Renderer::CleanUpSkybox(b32 justSemaphores)
{
  if (!justSemaphores) {
    DescriptorSet* skyboxSet = skybox_descriptorSetKey;
    m_pRhi->FreeDescriptorSet(skyboxSet);
  }
  for (u32 i = 0; i < m_SkyboxFinishedSignals.size(); ++i) {
    m_pRhi->FreeVkSemaphore(m_SkyboxFinishedSignals[i]);
    m_SkyboxFinishedSignals[i] = nullptr;
  }

  for (u32 i = 0; i < m_pSkyboxCmdBuffers.size(); ++i) {
    // Cleanup commandbuffer for skybox.
    m_pRhi->FreeCommandBuffer(m_pSkyboxCmdBuffers[i]);
    m_pSkyboxCmdBuffers[i] = nullptr;
  }
}


void Renderer::GenerateSkyboxCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{
  R_TIMED_PROFILE_RENDERER();
  
  CommandBuffer* buf = cmdBuffer;
  FrameBuffer* skyFrameBuffer = pbr_FrameBufferKey;
  GraphicsPipeline* skyPipeline = skybox_pipelineKey;
  DescriptorSet* global = m_pGlobal->Set(frameIndex);
  DescriptorSet* skybox = skybox_descriptorSetKey;

  VkDescriptorSet descriptorSets[] = {
    global->Handle(),
    skybox->Handle()
  };  

  std::array<VkClearValue, 3> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].depthStencil = { 1.0f, 0 };

  VkExtent2D windowExtent = { 
    (u32)m_pGlobal->Data()->_ScreenSize[0], 
    (u32)m_pGlobal->Data()->_ScreenSize[1] 
  };
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
    buf->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->Pipeline());
    buf->SetViewPorts(0, 1, &viewport);
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
}


void Renderer::GenerateOffScreenCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{
  R_TIMED_PROFILE_RENDERER();

  if (!m_pLights || !m_pGlobal) {  
    Log(rWarning) << "Can not build commandbuffers without light or global data! One of them is null!";
  } 

  FrameBuffer* gbuffer_FrameBuffer = gbuffer_FrameBufferKey;
  GraphicsPipeline* gbuffer_Pipeline = gbuffer_PipelineKey;
  GraphicsPipeline* gbuffer_StaticPipeline = gbuffer_StaticPipelineKey;

  std::array<VkClearValue, 5> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[4].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo gbuffer_RenderPassInfo = {};
  VkExtent2D windowExtent = { 
    (u32)m_pGlobal->Data()->_ScreenSize[0], 
    (u32)m_pGlobal->Data()->_ScreenSize[1] 
  };
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
  
  cmdBuffer->BeginRenderPass(gbuffer_RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    for (size_t i = 0; i < m_cmdDeferredList.Size(); ++i) {
      PrimitiveRenderCmd& renderCmd = m_cmdDeferredList.Get(i);
      // Need to notify that this render command does not have a render object.
      if (!renderCmd._pMeshDesc) continue;
      if (!(renderCmd._config & CMD_RENDERABLE_BIT) ||
          (renderCmd._config & (CMD_TRANSPARENT_BIT | CMD_TRANSLUCENT_BIT))) continue;
      R_ASSERT(renderCmd._pMeshData, "Null data passed to renderer.");

      MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
      // Set up the render mesh
      MeshData* data = renderCmd._pMeshData;

      b32 Skinned = (renderCmd._config & CMD_SKINNED_BIT);
      GraphicsPipeline* Pipe = Skinned ? gbuffer_Pipeline : gbuffer_StaticPipeline;
      VertexBuffer* vertexBuffer = data->VertexData();
      IndexBuffer* indexBuffer = data->IndexData();
      VkBuffer vb = vertexBuffer->Handle()->NativeBuffer();
      VkDeviceSize offsets[] = { 0 };
      cmdBuffer->BindVertexBuffers(0, 1, &vb, offsets);
      if (renderCmd._config & CMD_MORPH_BIT) {
        Pipe = Skinned ? gbuffer_morphTargetPipeline : gbuffer_staticMorphTargetPipeline;
        R_ASSERT(renderCmd._pMorph0, "morph0 is null");
        R_ASSERT(renderCmd._pMorph1, "morph1 is null.");
        VkBuffer morph0 = renderCmd._pMorph0->VertexData()->Handle()->NativeBuffer();
        VkBuffer morph1 = renderCmd._pMorph1->VertexData()->Handle()->NativeBuffer();
        cmdBuffer->BindVertexBuffers(1, 1, &morph0, offsets);
        cmdBuffer->BindVertexBuffers(2, 1,  &morph1, offsets);
      } 

      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipe->Pipeline());
      cmdBuffer->SetViewPorts(0, 1, &viewport);

      DescriptorSets[0] = m_pGlobal->Set(frameIndex)->Handle();
      DescriptorSets[1] = pMeshDesc->CurrMeshSet(frameIndex)->Handle();
      DescriptorSets[3] = (Skinned ? renderCmd._pJointDesc->CurrJointSet(frameIndex)->Handle() : nullptr);

      if (indexBuffer) {
        VkBuffer ib = indexBuffer->Handle()->NativeBuffer();
        cmdBuffer->BindIndexBuffer(ib, 0, GetNativeIndexType(indexBuffer->GetSizeType()));
      }

      MaterialDescriptor* pMatDesc = renderCmd._pPrimitive->_pMat->Native();
      DescriptorSets[2] = pMatDesc->CurrMaterialSet()->Handle();
      // Bind materials.
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, 
        Pipe->Layout(), 0, (Skinned ? 4 : 3), DescriptorSets, 0, nullptr);
      if (indexBuffer) {
        cmdBuffer->DrawIndexed(renderCmd._pPrimitive->_indexCount, renderCmd._instances, 
          renderCmd._pPrimitive->_firstIndex, 0, 0);
      } else {
        cmdBuffer->Draw(vertexBuffer->VertexCount(), renderCmd._instances, 0, 0);
      }
    }
  cmdBuffer->EndRenderPass();

  // Build decals after.
  m_decalEngine->BuildDecals(cmdBuffer);
}


void Renderer::GenerateFinalCmds(CommandBuffer* cmdBuffer)
{
  R_TIMED_PROFILE_RENDERER();

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
}


void Renderer::GenerateHDRCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{


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

  if (m_currentGraphicsConfigs._EnableBloom) {
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
  }

  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  VkDescriptorSet GlowDescriptorNative = GlowSet->Handle();
  cmdBuffer->BeginRenderPass(GlowPass, VK_SUBPASS_CONTENTS_INLINE);
  if (!m_currentGraphicsConfigs._EnableBloom) {
    VkClearAttachment clearAttachment = {};
    clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clearAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearAttachment.colorAttachment = 0;
    VkClearRect rect = {};
    rect.baseArrayLayer = 0;
    rect.layerCount = 1;
    VkExtent2D extent = m_pRhi->SwapchainObject()->SwapchainExtent();
    rect.rect.extent = extent;
    rect.rect = { 0, 0 };
    cmdBuffer->ClearAttachments(1, &clearAttachment, 1, &rect);
  } else {
    cmdBuffer->SetViewPorts(0, 1, &viewport);
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Pipeline());
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Layout(), 0, 1, &GlowDescriptorNative, 0, nullptr);
    cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmdBuffer->BindIndexBuffer(indexBuffer, 0, indexType);
    cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
  }
  cmdBuffer->EndRenderPass();

  

  VkDescriptorSet dSets[3];
  dSets[0] = m_pGlobal->Set(frameIndex)->Handle();
  dSets[1] = hdrSet->Handle();
  dSets[2] = m_pHDR->GetSet()->Handle();
  
  if (m_currentGraphicsConfigs._AA == AA_FXAA_2x) {
    m_pAntiAliasingFXAA->GenerateCommands(m_pRhi, cmdBuffer, m_pGlobal, frameIndex);
  }

  cmdBuffer->BeginRenderPass(renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->SetViewPorts(0, 1, &viewport);
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Pipeline());
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Layout(), 0, 3, dSets, 0, nullptr);
    cmdBuffer->PushConstants(hdrPipeline->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ParamsHDR), &m_HDR._pushCnst);
    cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmdBuffer->BindIndexBuffer(indexBuffer, 0, indexType);
    cmdBuffer->DrawIndexed(m_RenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
  cmdBuffer->EndRenderPass();
}


void Renderer::AdjustHDRSettings(const ParamsHDR& hdrSettings)
{
  m_HDR._pushCnst = hdrSettings;
  for (u32 i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_HDR._CmdBuffers[i];
    R_ASSERT(pCmdBuffer, "HDR buffer is null");
    pCmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo cmdBi = { };
    cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  
    pCmdBuffer->Begin(cmdBi);
    GenerateHDRCmds(pCmdBuffer, i);
    pCmdBuffer->End();
  }
}


void Renderer::GenerateShadowCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{
  ShadowMapSystem& system = m_pLights->PrimaryShadowMapSystem();
  system.GenerateDynamicShadowCmds(cmdBuffer, m_dynamicCmdList, frameIndex);
  if (system.StaticMapNeedsUpdate()) {
    system.GenerateStaticShadowCmds(cmdBuffer, m_staticCmdList, frameIndex);
    SignalStaticUpdate();
  }
#if 0 
  R_TIMED_PROFILE_RENDERER();
  if (!m_pLights) return;
  if (!m_pLights->m_pFrameBuffer) return;

  GraphicsPipeline* staticPipeline = ShadowMapPipelineKey;
  GraphicsPipeline* dynamicPipeline = DynamicShadowMapPipelineKey;  
  DescriptorSet*    lightViewSet = m_pLights->m_pLightViewDescriptorSet;  

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
    R_ASSERT(renderCmd._pMeshData, "Null data was passed to renderer.");
    MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
    b32 skinned = (renderCmd._config & CMD_SKINNED_BIT);
    VkDescriptorSet descriptorSets[4];
    descriptorSets[0] = pMeshDesc->CurrMeshSet()->Handle();
    descriptorSets[1] = lightViewSet->Handle();
    descriptorSets[3] = skinned ? renderCmd._pJointDesc->CurrJointSet()->Handle() : VK_NULL_HANDLE;

    GraphicsPipeline* pipeline = skinned ? dynamicPipeline : staticPipeline;
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Pipeline());
    cmdBuffer->SetViewPorts(0, 1, &viewport);
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

    Primitive* primitives = mesh->GetPrimitiveData();
    u32 count = mesh->GetPrimitiveCount();
    for (u32 i = 0; i < count; ++i) {
      Primitive& primitive = primitives[i];
      descriptorSets[2] = primitive._pMat->CurrMaterialSet()->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Layout(), 0, skinned ? 4 : 3, descriptorSets, 0, nullptr);
      if (index) {
        cmdBuffer->DrawIndexed(primitive._indexCount, renderCmd._instances, primitive._firstIndex, 0, 0);
      } else {
        cmdBuffer->Draw(primitive._indexCount, renderCmd._instances, primitive._firstIndex, 0);
      }
    }
  };

  // Create the shadow rendering pass.
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
#endif 
}


void Renderer::SetUpForwardPBR()
{
  m_Forward._cmdBuffers.resize(m_pRhi->BufferingCount());
  m_Forward._semaphores.resize(m_pRhi->BufferingCount());
  VkSemaphoreCreateInfo semaCi = {};
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (u32 i = 0; i < m_Forward._cmdBuffers.size(); ++i) {
    m_Forward._cmdBuffers[i] = m_pRhi->CreateCommandBuffer();
    CommandBuffer* cmdB = m_Forward._cmdBuffers[i];
    cmdB->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  
    m_Forward._semaphores[i] = m_pRhi->CreateVkSemaphore();
    m_Forward._semaphores[i]->Initialize(semaCi);
  }

}


void Renderer::CleanUpForwardPBR()
{
  for (u32 i = 0; i < m_Forward._cmdBuffers.size(); ++i) {
    m_pRhi->FreeCommandBuffer(m_Forward._cmdBuffers[i]);
    m_pRhi->FreeVkSemaphore(m_Forward._semaphores[i]);
  }
}


void Renderer::GenerateForwardPBRCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{
  R_TIMED_PROFILE_RENDERER();

  GraphicsPipeline* staticPipeline = pbr_staticForwardPipeline_NoLR;
  GraphicsPipeline* skinPipeline = pbr_forwardPipeline_NoLR;
  GraphicsPipeline* skinMorphPipeline = pbr_forwardPipelineMorphTargets_NoLR;
  GraphicsPipeline* staticMorphPipeline = pbr_staticForwardPipelineMorphTargets_NoLR;
  GraphicsPipeline* staticPipelineDebug = pbr_static_NoLR_Debug;
  GraphicsPipeline* skinPipelineDebug = pbr_dynamic_NoLR_Debug;
  GraphicsPipeline* staticMorphPipelineDebug = pbr_static_mt_NoLR_Debug;
  GraphicsPipeline* skinMorphPipelineDebug = pbr_dynamic_NoLR_mt_Debug;

  if (m_currentGraphicsConfigs._EnableLocalReflections) {
    staticPipeline = pbr_staticForwardPipeline_LR;
    skinPipeline = pbr_forwardPipeline_LR;
    skinMorphPipeline = pbr_forwardPipelineMorphTargets_LR;
    staticMorphPipeline = pbr_staticForwardPipelineMorphTargets_LR;
    staticPipelineDebug = pbr_static_LR_Debug;
    skinPipelineDebug = pbr_dynamic_LR_Debug;
    staticMorphPipelineDebug = pbr_static_mt_LR_Debug;
    skinMorphPipelineDebug = pbr_dynamic_LR_mt_Debug;
  }
  std::array<VkClearValue, 7> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[4].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[5].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[6].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo renderPassCi = {};
  VkExtent2D windowExtent = { 
    (u32)m_pGlobal->Data()->_ScreenSize[0], 
    (u32)m_pGlobal->Data()->_ScreenSize[1] 
  };
  renderPassCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassCi.framebuffer = pbr_forwardFrameBuffer->Handle();
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
  
  VkDescriptorSet DescriptorSets[8];

  cmdBuffer->BeginRenderPass(renderPassCi, VK_SUBPASS_CONTENTS_INLINE);
    for (size_t i = 0; i < m_forwardCmdList.Size(); ++i) {
      PrimitiveRenderCmd& renderCmd = m_forwardCmdList[i];
      if (!(renderCmd._config & CMD_FORWARD_BIT) && !(renderCmd._config & CMD_RENDERABLE_BIT)) continue;
      R_ASSERT(renderCmd._pMeshData, "Null mesh data was passed to renderer.");
      MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
      b32 Skinned = (renderCmd._config & CMD_SKINNED_BIT);
      b32 debugging = (renderCmd._config & CMD_DEBUG_BIT);
      // Set up the render mesh
      MeshData* data = renderCmd._pMeshData;

      VertexBuffer* vertexBuffer = data->VertexData();
      IndexBuffer* indexBuffer = data->IndexData();
      VkBuffer vb = vertexBuffer->Handle()->NativeBuffer();
      VkDeviceSize offsets[] = { 0 };

      GraphicsPipeline* Pipe = (Skinned ? 
        (debugging ? skinPipelineDebug : skinPipeline) 
        : (debugging ? staticPipelineDebug : staticPipeline));
      cmdBuffer->BindVertexBuffers(0, 1, &vb, offsets);
      if (renderCmd._config & CMD_MORPH_BIT) {
        Pipe = (Skinned ? 
          (debugging ? skinMorphPipelineDebug : skinMorphPipeline) 
          : (debugging ? staticMorphPipelineDebug : staticMorphPipeline));
        R_ASSERT(renderCmd._pMorph0, "morph0 is null");
        R_ASSERT(renderCmd._pMorph1, "morph1 is null.");
        VkBuffer morph0 = renderCmd._pMorph0->VertexData()->Handle()->NativeBuffer();
        VkBuffer morph1 = renderCmd._pMorph1->VertexData()->Handle()->NativeBuffer();
        cmdBuffer->BindVertexBuffers(1, 1, &morph0, offsets);
        cmdBuffer->BindVertexBuffers(2, 1, &morph1, offsets);
      }
  
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipe->Pipeline());
      cmdBuffer->SetViewPorts(0, 1, &viewport);

      if (debugging) {
        struct ivec4 {
          i32 v[4];
        };
        ivec4 value;
        value.v[0] = renderCmd._debugConfig;
        cmdBuffer->PushConstants(Pipe->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ivec4), &value);
      }

      if (indexBuffer) {
        VkBuffer ib = indexBuffer->Handle()->NativeBuffer();
        cmdBuffer->BindIndexBuffer(ib, 0, GetNativeIndexType(indexBuffer->GetSizeType()));
      }
      ShadowMapSystem& shadow = m_pLights->PrimaryShadowMapSystem();
      DescriptorSets[0] = m_pGlobal->Set(frameIndex)->Handle();
      DescriptorSets[1] = pMeshDesc->CurrMeshSet(frameIndex)->Handle();
      DescriptorSets[3] = m_pLights->Set()->Handle();
      DescriptorSets[4] = shadow.ShadowMapViewDescriptor(frameIndex)->Handle();
      DescriptorSets[5] = shadow.StaticShadowMapViewDescriptor(frameIndex)->Handle();
      DescriptorSets[6] = m_pGlobalIllumination->GetDescriptorSet()->Handle(); // Need global illumination data.
      DescriptorSets[7] = (Skinned ? renderCmd._pJointDesc->CurrJointSet(frameIndex)->Handle() : nullptr);

      // Bind materials.
      Primitive* primitive = renderCmd._pPrimitive;
      DescriptorSets[2] = primitive->_pMat->Native()->CurrMaterialSet()->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
        Pipe->Layout(),
        0,
        (Skinned ? 8 : 7),
        DescriptorSets,
        0,
        nullptr
      );

      if (indexBuffer) {
        cmdBuffer->DrawIndexed(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0, 0);
      } else {
        cmdBuffer->Draw(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0);
      }
    }
  cmdBuffer->EndRenderPass();

  // TODO(): Move particle compute on separate job, can be ran asyncronously!
  m_particleEngine->GenerateParticleComputeCommands(m_pRhi, cmdBuffer, m_pGlobal, m_particleSystems, frameIndex);
  m_particleEngine->GenerateParticleRenderCommands(m_pRhi, cmdBuffer, m_pGlobal, m_particleSystems, frameIndex);
}


void Renderer::SetUpGlobalIlluminationBuffer()
{
  m_pGlobalIllumination->Initialize(m_pRhi, m_currentGraphicsConfigs._EnableLocalReflections);
}


void Renderer::UpdateGlobalIlluminationBuffer()
{
  Texture* pCube = m_pSky->GetCubeMap();
  Texture* pTex = nullptr;
  if (m_usePreRenderSkybox) {
    pCube = m_preRenderSkybox->Handle();
  }
  if (m_skybox._brdfLUT) {
    pTex = m_skybox._brdfLUT->Handle();
  }
  m_pGlobalIllumination->SetGlobalProbe(m_globalLightProbe);
  m_pGlobalIllumination->SetGlobalEnvMap(pCube);
  m_pGlobalIllumination->SetGlobalBRDFLUT(pTex);
  m_pGlobalIllumination->Update(this);
}


void Renderer::CleanUpGlobalIlluminationBuffer()
{
  m_pGlobalIllumination->CleanUp(m_pRhi);
}


void Renderer::SetUpPBR()
{
  Sampler* pbr_Sampler = gbuffer_SamplerKey;

  DescriptorSetLayout* pbr_Layout = pbr_DescLayoutKey;
  if ( pbr_DescSetKey ) {
    m_pRhi->FreeDescriptorSet( pbr_DescSetKey );
    pbr_DescSetKey = nullptr;
  }
  DescriptorSet* pbr_Set = m_pRhi->CreateDescriptorSet();
  pbr_DescSetKey = pbr_Set;
  {
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
    writeInfo[0].dstSet = nullptr;
    writeInfo[0].pImageInfo = &albedo;
    writeInfo[0].pBufferInfo = nullptr;
    writeInfo[0].pTexelBufferView = nullptr;
    writeInfo[0].dstArrayElement = 0;
    writeInfo[0].pNext = nullptr;

    writeInfo[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo[1].descriptorCount = 1;
    writeInfo[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeInfo[1].dstBinding = 1;
    writeInfo[1].dstSet = nullptr;
    writeInfo[1].pImageInfo = &normal;
    writeInfo[1].pBufferInfo = nullptr;
    writeInfo[1].pTexelBufferView = nullptr;
    writeInfo[1].dstArrayElement = 0;
    writeInfo[1].pNext = nullptr;

    writeInfo[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo[2].descriptorCount = 1;
    writeInfo[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeInfo[2].dstBinding = 2;
    writeInfo[2].dstSet = nullptr;
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
  }

  if (pbr_compSet) {
    m_pRhi->FreeDescriptorSet(pbr_compSet);
    pbr_compSet = nullptr;
  }
  pbr_compSet = m_pRhi->CreateDescriptorSet();
  {
    VkDescriptorImageInfo outResult = { };
    outResult.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    outResult.imageView = pbr_FinalTextureKey->View();
    outResult.sampler = nullptr;    

    VkDescriptorImageInfo outBright = { };
    outBright.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    outBright.imageView = pbr_BrightTextureKey->View();
    outBright.sampler = nullptr;

    std::array<VkWriteDescriptorSet, 2> writes;
    writes[0] = { };
    writes[1] = { };

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pImageInfo = &outResult;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[0].descriptorCount = 1;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].pImageInfo = &outBright;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    pbr_compSet->Allocate(m_pRhi->DescriptorPool(), pbr_compDescLayout);
    pbr_compSet->Update(static_cast<u32>(writes.size()), writes.data());
  }

  for (u32 i = 0; i < m_Pbr._CmdBuffers.size(); ++i) {
    if (m_Pbr._CmdBuffers[i]) {
      m_pRhi->FreeCommandBuffer(m_Pbr._CmdBuffers[i]);
      m_Pbr._CmdBuffers[i] = nullptr;
    }
    if (m_Pbr._Semas[i]) {
      m_pRhi->FreeVkSemaphore(m_Pbr._Semas[i]);
      m_Pbr._Semas[i] = nullptr;
    }
  }

  m_Pbr._CmdBuffers.resize(m_pRhi->BufferingCount());
  m_Pbr._Semas.resize(m_pRhi->BufferingCount());

  VkSemaphoreCreateInfo semaCi = {};
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (u32 i = 0; i < m_Pbr._Semas.size(); ++i) {
    m_Pbr._Semas[i] = m_pRhi->CreateVkSemaphore();
    m_Pbr._Semas[i]->Initialize(semaCi);
    m_Pbr._CmdBuffers[i] = m_pRhi->CreateCommandBuffer();
    m_Pbr._CmdBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  }
}


void Renderer::CleanUpPBR()
{
  DescriptorSet* pbr_Set = pbr_DescSetKey;
  m_pRhi->FreeDescriptorSet(pbr_Set);
  m_pRhi->FreeDescriptorSet(pbr_compSet);

  pbr_DescSetKey = nullptr;
  pbr_compSet = nullptr;

  for (u32 i = 0; i < m_Pbr._Semas.size(); ++i) {
    m_pRhi->FreeVkSemaphore(m_Pbr._Semas[i]);
    m_pRhi->FreeCommandBuffer(m_Pbr._CmdBuffers[i]);
    m_Pbr._Semas[i] = nullptr;
    m_Pbr._CmdBuffers[i] = nullptr;
  }
}


void Renderer::SetUpFinalOutputs()
{
  Texture* pbr_Final = pbr_FinalTextureKey;
  Texture* hdr_Color = hdr_gamma_colorAttachKey;

  Sampler* hdr_Sampler = hdr_gamma_samplerKey;
  Sampler* pbr_Sampler = gbuffer_SamplerKey;

  DescriptorSetLayout* finalSetLayout = final_DescSetLayoutKey;
  if ( final_DescSetKey ) {
    m_pRhi->FreeDescriptorSet( final_DescSetKey );
    final_DescSetKey = nullptr; 
  }
  DescriptorSet* offscreenImageDescriptor = m_pRhi->CreateDescriptorSet();
  final_DescSetKey = offscreenImageDescriptor;
  offscreenImageDescriptor->Allocate(m_pRhi->DescriptorPool(), finalSetLayout);
  {
    // Final texture must be either hdr post process texture, or pbr output without hdr.
    VkDescriptorImageInfo renderTextureFinal = {};
    renderTextureFinal.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
    renderTextureOut.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

  for (u32 i = 0; i < m_pFinalFinishedSemas.size(); ++i) {
    if (m_pFinalFinishedSemas[i]) {
      m_pRhi->FreeVkSemaphore(m_pFinalFinishedSemas[i]);
      m_pFinalFinishedSemas[i] = nullptr;
    }
    if (m_pFinalCommandBuffers[i]) {
      m_pRhi->FreeCommandBuffer(m_pFinalCommandBuffers[i]);
      m_pFinalCommandBuffers[i] = nullptr;
    }
  }

  m_pFinalFinishedSemas.resize(m_pRhi->BufferingCount());
  m_pFinalCommandBuffers.resize(m_pRhi->BufferingCount());
  VkSemaphoreCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (u32 i = 0; i < m_pFinalFinishedSemas.size(); ++i) {
    m_pFinalFinishedSemas[i] = m_pRhi->CreateVkSemaphore();
    m_pFinalFinishedSemas[i]->Initialize(info); 
    m_pFinalCommandBuffers[i] = m_pRhi->CreateCommandBuffer();
    m_pFinalCommandBuffers[i]->Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  }
}


void Renderer::CheckCmdUpdate()
{
  R_TIMED_PROFILE_RENDERER();

  i32 frameIndex = m_pRhi->CurrentFrame();
  VkCommandBufferBeginInfo begin{};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (m_currentGraphicsConfigs._EnableMultithreadedRendering) {
    m_workers[0] = std::thread([&]() -> void {
      CommandBuffer* offscreenCmdList = m_Offscreen._cmdBuffers[frameIndex];
      offscreenCmdList->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      offscreenCmdList->Begin(begin);
      GenerateOffScreenCmds(offscreenCmdList, frameIndex);
      offscreenCmdList->End();

      m_Forward._cmdBuffers[frameIndex]->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

      m_Forward._cmdBuffers[frameIndex]->Begin(begin);
      GenerateForwardPBRCmds(m_Forward._cmdBuffers[frameIndex], frameIndex);
      m_Forward._cmdBuffers[frameIndex]->End();
    });

    if (m_pLights->PrimaryShadowEnabled() || m_pLights->PrimaryShadowMapSystem().StaticMapNeedsUpdate()) {
      CommandBuffer* shadowBuf = m_Offscreen._shadowCmdBuffers[frameIndex];
      R_ASSERT(shadowBuf, "Shadow Buffer is null.");
      shadowBuf->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      shadowBuf->Begin(begin);
      GenerateShadowCmds(shadowBuf, frameIndex);
      shadowBuf->End();
    }

    m_pUI->BuildCmdBuffers(m_pGlobal, frameIndex);
    m_workers[0].join();
  } else {
    CommandBuffer* offscreenCmdList = m_Offscreen._cmdBuffers[frameIndex];
    offscreenCmdList->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    offscreenCmdList->Begin(begin);
    GenerateOffScreenCmds(offscreenCmdList, frameIndex);
    offscreenCmdList->End();

    m_Forward._cmdBuffers[frameIndex]->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    m_Forward._cmdBuffers[frameIndex]->Begin(begin);
    GenerateForwardPBRCmds(m_Forward._cmdBuffers[frameIndex], frameIndex);
    m_Forward._cmdBuffers[frameIndex]->End();

    if (m_pLights->PrimaryShadowEnabled() || m_pLights->PrimaryShadowMapSystem().StaticMapNeedsUpdate()) {
      CommandBuffer* shadowBuf = m_Offscreen._shadowCmdBuffers[frameIndex];
      R_ASSERT(shadowBuf, "Shadow Buffer is null.");
      shadowBuf->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      shadowBuf->Begin(begin);
      GenerateShadowCmds(shadowBuf, frameIndex);
      shadowBuf->End();
    }
    m_pUI->BuildCmdBuffers(m_pGlobal, frameIndex);
  }

#if 0
  if (m_NeedsUpdate) {
   if (m_AsyncBuild) {
      BuildAsync();
    } else {
      Build();
    }
  }
#endif
}


void Renderer::RenderPrimaryShadows()
{

}


void Renderer::CleanUpFinalOutputs()
{
  DescriptorSet* offscreenDescriptorSet = final_DescSetKey;
  m_pRhi->FreeDescriptorSet(offscreenDescriptorSet);

  final_DescSetKey = nullptr;
  for (u32 i = 0; i < m_pFinalFinishedSemas.size(); ++i) {
    m_pRhi->FreeVkSemaphore(m_pFinalFinishedSemas[i]);
    m_pFinalFinishedSemas[i] = nullptr;
    m_pRhi->FreeCommandBuffer(m_pFinalCommandBuffers[i]);
    m_pFinalCommandBuffers[i] = nullptr;
  }
}


void Renderer::UpdateSceneDescriptors(u32 frameIndex)
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
    m_pLights->PrimaryShadowMapSystem().SignalStaticMapUpdate();
  }

  // Update the global descriptor.
  m_pGlobal->Update(m_pRhi, frameIndex);

  // Update lights in scene.
#if 0
  Vector4 vViewerPos = m_pGlobal->Data()->_CameraPos;
  m_pLights->SetViewerPosition(Vector3(vViewerPos.x, vViewerPos.y, vViewerPos.z));
#endif
  m_pLights->Update(m_pRhi, m_pGlobal->Data(), frameIndex);

  // Update mesh descriptors.
  for (size_t i = 0; i < m_meshDescriptors.Size(); ++i) {
    MeshDescriptor* descriptor = m_meshDescriptors[i];
    descriptor->Update(m_pRhi, frameIndex);
  }
  
  // Update material descriptors.
  for (size_t i = 0; i < m_materialDescriptors.Size(); ++i) {
    MaterialDescriptor* descriptor = m_materialDescriptors[i];
    R_ASSERT(descriptor, "Null material descriptor.");
    descriptor->Update(m_pRhi);
  }

  // Update Joint descriptors.
  for (size_t i = 0; i < m_jointDescriptors.Size(); ++i) {
    JointDescriptor* descriptor = m_jointDescriptors[i];
    descriptor->Update(m_pRhi, frameIndex);
  }

  for (size_t i = 0; i < m_particleSystems.Size(); ++i) {
    ParticleSystem* system = m_particleSystems[i];
    system->Update(m_pRhi);
  }

  // Update realtime hdr settings.
  m_pHDR->UpdateToGPU(m_pRhi);
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
    case GRAPHICS_QUALITY_NONE:
    {
      m_pLights->EnablePrimaryShadow(false);
      m_pGlobal->Data()->_EnableShadows = false;
    } break;
    case GRAPHICS_QUALITY_POTATO:
    case GRAPHICS_QUALITY_LOW:
    case GRAPHICS_QUALITY_MEDIUM:
    case GRAPHICS_QUALITY_HIGH:
    case GRAPHICS_QUALITY_ULTRA:
    default:
    {
      m_pLights->EnablePrimaryShadow(true);
      m_pGlobal->Data()->_EnableShadows = true;
    } break;
  }

  ShadowMapSystem& sunShadow = m_pLights->PrimaryShadowMapSystem();
  sunShadow.EnableDynamicMapSoftShadows(params->_EnableSoftShadows);
  sunShadow.EnableStaticMapSoftShadows(params->_EnableSoftShadows);

  m_pHDR->GetRealtimeConfiguration()->_allowChromaticAberration = 
    (params->_EnableChromaticAberration ? Vector4(1.0f) : Vector4(0.0f));
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
  u32 bufferCount = m_pRhi->BufferingCount();
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

    if (m_currentGraphicsConfigs._EnableLocalReflections != params->_EnableLocalReflections) {
      reconstruct = true;
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
    m_pGlobal->Data()->_ScreenSize[0] = m_pWindow->Width();
    m_pGlobal->Data()->_ScreenSize[1] = m_pWindow->Height();
    // Triple buffering atm, we will need to use user params to switch this.
    m_pRhi->ReConfigure(presentMode, m_pWindow->Width(), m_pWindow->Height(), bufferCount, 3);

    m_pUI->CleanUp();

    CleanUpForwardPBR();
    CleanUpPBR();
    CleanUpHDR(false);
    CleanUpDownscale(false);
    CleanUpSkybox(true);
    //CleanUpOffscreen();
    CleanUpFinalOutputs();
    CleanUpGraphicsPipelines();
    CleanUpFrameBuffers();
    CleanUpRenderTextures(false);
    m_particleEngine->CleanUpPipeline(m_pRhi);

    SetUpRenderTextures(false);
    SetUpFrameBuffers();
    SetUpGraphicsPipelines();
    SetUpForwardPBR();
    m_particleEngine->InitializePipeline(m_pRhi);
    m_pUI->Initialize(m_pRhi);
  }

  SetUpOffscreen();
  SetUpDownscale(false);
  SetUpHDR(false);
  SetUpPBR();
  SetUpSkybox(true);
  SetUpFinalOutputs();

  CleanUpGlobalIlluminationBuffer();
  SetUpGlobalIlluminationBuffer();
  UpdateGlobalIlluminationBuffer();

  Build();
  m_pLights->PrimaryShadowMapSystem().SignalStaticMapUpdate();
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
  texture->mRhi = m_pRhi;
  return texture;
}


void Renderer::FreeTexture2DArray(Texture2DArray* texture)
{
  if (!texture) return;
  texture->CleanUp();
  delete texture;
}

void Renderer::FreeTextureCube(TextureCube* texture)
{
  if (!texture) return;
  texture->CleanUp();
  delete texture;
}


Texture3D* Renderer::CreateTexture3D()
{
  Texture3D* texture = new Texture3D();
  return texture;
}


MaterialDescriptor* Renderer::CreateMaterialDescriptor()
{
  MaterialDescriptor* descriptor = new MaterialDescriptor();
  return descriptor;
}


void Renderer::FreeMaterialDescriptor(MaterialDescriptor* descriptor)
{
  if (!descriptor) return;
  descriptor->CleanUp(m_pRhi);
  delete descriptor;
}


TextureCube* Renderer::CreateTextureCube()
{
  TextureCube* pCube = new TextureCube();
  pCube->mRhi = m_pRhi;
  return pCube;
}


MeshDescriptor* Renderer::CreateMeshDescriptor()
{
  MeshDescriptor* descriptor = new MeshDescriptor();
  return descriptor;
}


JointDescriptor* Renderer::CreateJointDescriptor()
{
  JointDescriptor* descriptor = new JointDescriptor();
  return descriptor;
}


TextureSampler* Renderer::CreateTextureSampler(const SamplerInfo& info)
{
  TextureSampler* pSampler = new TextureSampler();
  pSampler->Initialize(m_pRhi, info);
  return pSampler;
}


void Renderer::FreeJointDescriptor(JointDescriptor* descriptor)
{
  if (!descriptor) return;
  descriptor->CleanUp(m_pRhi);
  delete descriptor;
}


void Renderer::FreeTextureSampler(TextureSampler* pSampler)
{
  if (!pSampler) return; 
  pSampler->CleanUp(m_pRhi);
  delete pSampler;
}


void Renderer::FreeMeshDescriptor(MeshDescriptor* descriptor)
{
  if (!descriptor) return;
  descriptor->CleanUp(m_pRhi);
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
  R_TIMED_PROFILE_RENDERER();

  m_cmdDeferredList.Sort();
  m_forwardCmdList.Sort();
  // TODO(): Also sort forward list too.
}


void Renderer::ClearCmdLists()
{
  R_TIMED_PROFILE_RENDERER();

  // TODO(): Clear forward command list as well.
  m_cmdDeferredList.Clear();
  m_forwardCmdList.Clear();
  m_staticCmdList.Clear();
  m_dynamicCmdList.Clear();
  m_meshDescriptors.Clear();
  m_jointDescriptors.Clear();
  m_materialDescriptors.Clear();
  m_particleSystems.Clear();
}


void Renderer::PushMeshRender(MeshRenderCmd& cmd)
{
  if (m_Minimized) return;

  Primitive* primitives = cmd._pPrimitives;
  u32 count = cmd._primitiveCount;
  for (u32 i = 0; i < count; ++i) {
    Primitive& prim = primitives[i];
    PrimitiveRenderCmd primCmd = {};
    primCmd._config = cmd._config | prim._localConfigs;
    primCmd._pJointDesc = cmd._pJointDesc;
    primCmd._pMeshData = cmd._pMeshData;
    primCmd._pMeshDesc = cmd._pMeshDesc;
    primCmd._pMorph0 = cmd._pMorph0;
    primCmd._pMorph1 = cmd._pMorph1;
    primCmd._pPrimitive = &prim;
    primCmd._instances = 1;
    primCmd._debugConfig = cmd._debugConfig;

    if (primCmd._config & ~CMD_BASIC_RENDER_BIT) {
      R_ASSERT(prim._pMat, "No material descriptor added to this primitive. Need to set a material descriptor!");
      m_materialDescriptors.PushBack(prim._pMat->Native());
      R_ASSERT(prim._pMat->Native() == m_materialDescriptors[m_materialDescriptors.Size() - 1], "Corrupted material descriptors.");
    }
    if (primCmd._config & CMD_SKINNED_BIT) {
      R_ASSERT(cmd._pJointDesc, "No joint descriptoer added to this command.");
      m_jointDescriptors.PushBack(cmd._pJointDesc);
    }

    R_ASSERT(cmd._pMeshDesc, "No mesh descriptor added to this command.");
    m_meshDescriptors.PushBack(cmd._pMeshDesc);

    u32 config = primCmd._config;
    if ((config & (CMD_TRANSPARENT_BIT | CMD_TRANSLUCENT_BIT | CMD_FORWARD_BIT | CMD_DEBUG_BIT))) {
      m_forwardCmdList.PushBack(primCmd);
    }
    else {
      m_cmdDeferredList.PushBack(primCmd);
    }

    if ((config & CMD_STATIC_BIT)) {
      m_staticCmdList.PushBack(primCmd);
    }
    else {
      m_dynamicCmdList.PushBack(primCmd);
    }
  }
}


BufferUI* Renderer::GetUiBuffer() const
{
  return m_pUI->GetUIBuffer();
}


TextureCube* Renderer::BakeEnvironmentMap(const Vector3& position, u32 texSize)
{
  TextureCube* pTexCube = nullptr;
  if (texSize == 0) return pTexCube;

  pTexCube = new TextureCube();
  Texture* cubeTexture = m_pRhi->CreateTexture();
  pTexCube->mRhi = m_pRhi;

  {
    VkImageCreateInfo imageCi{};
    VkImageViewCreateInfo viewCi{};

    imageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCi.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCi.imageType = VK_IMAGE_TYPE_2D;
    imageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCi.arrayLayers = 6;
    imageCi.extent.depth = 1;
    imageCi.extent.width = texSize;
    imageCi.extent.height = texSize;
    imageCi.mipLevels = 1;
    imageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT 
      | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCi.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; 
    imageCi.samples = VK_SAMPLE_COUNT_1_BIT;

    viewCi.components = {};
    viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCi.subresourceRange.baseArrayLayer = 0;
    viewCi.subresourceRange.baseMipLevel = 0;
    viewCi.subresourceRange.layerCount = 6;
    viewCi.subresourceRange.levelCount = 1;
    viewCi.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    // Create the cube texture to be written onto.
    cubeTexture->Initialize(imageCi, viewCi, false);
  }


    // TODO():
    Matrix4 view;
    Matrix4 proj = Matrix4::Perspective(static_cast<r32>(CONST_PI_HALF), 1.0f, 0.1f, 512.0f);
    //proj[1][1] *= -1;
    GlobalBuffer* pGlobal = m_pGlobal->Data();
    i32 orx = pGlobal->_ScreenSize[0];
    i32 ory = pGlobal->_ScreenSize[1];
    Matrix4 prevView = pGlobal->_View;
    Matrix4 prevProj = pGlobal->_Proj;
    Matrix4 prevViewProj = pGlobal->_ViewProj;
    Matrix4 prevInvViewProj = pGlobal->_InvViewProj;
    Matrix4 prevInvView = pGlobal->_InvView;
    Matrix4 prevInvProj = pGlobal->_InvProj;
    Vector4 prevCameraPos = pGlobal->_CameraPos;
    pGlobal->_ScreenSize[0] = texSize;
    pGlobal->_ScreenSize[1] = texSize;

    pGlobal->_Proj = proj;
    pGlobal->_InvProj = proj.Inverse();
    CommandBuffer cmdBuffer;
    cmdBuffer.SetOwner(m_pRhi->LogicDevice()->Native());
    cmdBuffer.Allocate(m_pRhi->GraphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // TODO(): Signal a beginning and end callback or so, when performing 
    // any rendering.
    // Update the scene descriptors before rendering the frame.
    SortCmdLists();

    UpdateSceneDescriptors(0);

    std::array<Matrix4, 6> viewMatrices = {
      Quaternion::AngleAxis(Radians(-90.0f), Vector3(0.0f, 1.0f, 0.0f)).ToMatrix4(),
      Quaternion::AngleAxis(Radians(90.0f), Vector3(0.0f, 1.0f, 0.0f)).ToMatrix4(),

      Quaternion::AngleAxis(Radians(90.0f), Vector3(1.0f, 0.0f, 0.0f)).ToMatrix4(),
      Quaternion::AngleAxis(Radians(-90.0f), Vector3(1.0f, 0.0f, 0.0f)).ToMatrix4(),

      Quaternion::AngleAxis(Radians(0.0f), Vector3(0.0f, 1.0f, 0.0f)).ToMatrix4(),
      Quaternion::AngleAxis(Radians(180.0f), Vector3(0.0f, 1.0f, 0.0f)).ToMatrix4(),
    };

    // For each cube face.

    // TODO(): Fix view matrix issue, rendering skybox incorrectly!
    for (size_t i = 0; i < 6; ++i) {
      view = Matrix4::Translate(Matrix4(), -position) * viewMatrices[i];
      pGlobal->_CameraPos = Vector4(position, 1.0f);
      pGlobal->_View = view;
      pGlobal->_ViewProj = view * proj;
      pGlobal->_InvView = view.Inverse();
      pGlobal->_InvViewProj = pGlobal->_ViewProj.Inverse();
      m_pGlobal->Update(m_pRhi, 0);
      
      VkCommandBufferBeginInfo begin = { };
      begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      cmdBuffer.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      cmdBuffer.Begin(begin);

      // Render to frame first.
      GenerateOffScreenCmds(&cmdBuffer, 0);
      GenerateShadowCmds(&cmdBuffer, 0);
      GeneratePbrCmds(&cmdBuffer, 0);
      if (m_pSky->NeedsRendering()) {
        m_pSky->BuildCmdBuffer(m_pRhi, &cmdBuffer);
        m_pSky->MarkClean();
      }
      GenerateSkyboxCmds(&cmdBuffer, 0);
      GenerateForwardPBRCmds(&cmdBuffer, 0);

      cmdBuffer.End();

      VkSubmitInfo submit{};
      submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit.commandBufferCount = 1;
      VkCommandBuffer cmdBufs[] = { cmdBuffer.Handle() };
      submit.pCommandBuffers = cmdBufs;

      m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 1, &submit, VK_NULL_HANDLE);
      m_pRhi->GraphicsWaitIdle(DEFAULT_QUEUE_IDX);

      cmdBuffer.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      cmdBuffer.Begin(begin);
      VkDeviceSize offsets = { 0 };

      VkImageSubresourceRange subRange = {};
      subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      subRange.baseMipLevel = 0;
      subRange.baseArrayLayer = 0;
      subRange.levelCount = 1;
      subRange.layerCount = 6;

      VkImageMemoryBarrier imgMemBarrier = {};
      imgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imgMemBarrier.subresourceRange = subRange;
      imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      imgMemBarrier.srcAccessMask = 0;
      imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      imgMemBarrier.image = cubeTexture->Image();

      // set the cubemap image layout for transfer from our framebuffer.
      cmdBuffer.PipelineBarrier(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imgMemBarrier
      );

 
      subRange.baseArrayLayer = 0;
      subRange.baseMipLevel = 0;
      subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      subRange.layerCount = 1;
      subRange.levelCount = 1;

      imgMemBarrier.image = pbr_FinalTextureKey->Image();
      imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imgMemBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      imgMemBarrier.subresourceRange = subRange;

      // transfer color attachment to transfer.
      cmdBuffer.PipelineBarrier(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imgMemBarrier
      );

      VkImageCopy imgCopy = {};
      imgCopy.srcOffset = { 0, 0, 0 };
      imgCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imgCopy.srcSubresource.baseArrayLayer = 0;
      imgCopy.srcSubresource.mipLevel = 0;
      imgCopy.srcSubresource.layerCount = 1;

      imgCopy.dstOffset = { 0, 0, 0 };
      imgCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imgCopy.dstSubresource.baseArrayLayer = static_cast<u32>(i);
      imgCopy.dstSubresource.layerCount = 1;
      imgCopy.dstSubresource.mipLevel = 0;

      imgCopy.extent.width = texSize;
      imgCopy.extent.height = texSize;
      imgCopy.extent.depth = 1;

      cmdBuffer.CopyImage(
        pbr_FinalTextureKey->Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        cubeTexture->Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &imgCopy
      );

      imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      imgMemBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      cmdBuffer.PipelineBarrier(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imgMemBarrier
      );

      subRange.baseMipLevel = 0;
      subRange.baseArrayLayer = 0;
      subRange.levelCount = 1;
      subRange.layerCount = 6;

      imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      imgMemBarrier.image = cubeTexture->Image();
      imgMemBarrier.subresourceRange = subRange;

      cmdBuffer.PipelineBarrier(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imgMemBarrier
      );
      cmdBuffer.End();

      submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit.commandBufferCount = 1;
      submit.pCommandBuffers = cmdBufs;

      m_pRhi->GraphicsSubmit(DEFAULT_QUEUE_IDX, 1, &submit, VK_NULL_HANDLE);
      m_pRhi->GraphicsWaitIdle(DEFAULT_QUEUE_IDX);
    }

  pTexCube->SetTextureHandle(cubeTexture);

  pGlobal->_ScreenSize[0] = orx;
  pGlobal->_ScreenSize[1] = ory;
  pGlobal->_CameraPos = prevCameraPos;
  pGlobal->_View = prevView;
  pGlobal->_ViewProj = prevViewProj;
  pGlobal->_InvView = prevInvView;
  pGlobal->_InvViewProj = prevInvViewProj;
  return pTexCube;
}


void Renderer::TakeSnapshot(const std::string name)
{
  m_pRhi->WaitAllGraphicsQueues();
  Texture2D tex2d;
  tex2d.mRhi = m_pRhi;
  tex2d.texture = final_renderTargetKey;
  tex2d.Save(name);
}


Texture2D* Renderer::GenerateBRDFLUT(u32 x, u32 y)
{
  Texture2D* tex2D = new Texture2D();
  Texture* texture = m_pRhi->CreateTexture();
  tex2D->mRhi = m_pRhi;
  {
    VkImageCreateInfo imgCi = { };
    imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgCi.arrayLayers = 1;
    imgCi.extent.width = x;
    imgCi.extent.height = y;
    imgCi.extent.depth = 1;
    imgCi.format = VK_FORMAT_R8G8B8A8_UNORM;
    imgCi.imageType = VK_IMAGE_TYPE_2D;
    imgCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgCi.mipLevels = 1;
    imgCi.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgCi.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT 
      | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; 

    VkImageViewCreateInfo viewCi = { };
    viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCi.components = { };
    viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCi.subresourceRange.baseArrayLayer = 0;
    viewCi.subresourceRange.baseMipLevel = 0;
    viewCi.subresourceRange.layerCount = 1;
    viewCi.subresourceRange.levelCount = 1;
    viewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    texture->Initialize(imgCi, viewCi);
  }

  CommandBuffer cmd;
  cmd.SetOwner(m_pRhi->LogicDevice()->Native());
  cmd.Allocate(m_pRhi->ComputeCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = { };
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmd.Begin(beginInfo);

  GlobalBuffer* g = m_pGlobal->Data();
  i32 prevx = g->_ScreenSize[0];
  i32 prevy = g->_ScreenSize[1];
  g->_ScreenSize[0] = x;
  g->_ScreenSize[1] = y;
  m_pGlobal->Update(m_pRhi, 0);
  m_pBakeIbl->UpdateTargetBRDF(texture);
  m_pBakeIbl->RenderGenBRDF(&cmd, m_pGlobal, texture, 0);

  cmd.End();

  VkSubmitInfo submitInfo = { };
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer native[] = { cmd.Handle() };
  submitInfo.pCommandBuffers = native;

  m_pRhi->ComputeSubmit(0, submitInfo);
  m_pRhi->ComputeWaitIdle(0);


  g->_ScreenSize[0] = prevx;
  g->_ScreenSize[1] = prevy;
  m_pGlobal->Update(m_pRhi, 0);
  tex2D->texture = texture;
  return tex2D;
}


void Renderer::PushParticleSystem(ParticleSystem* system)
{
  if (!system) return;
  m_particleSystems.PushBack(system);
}


ParticleSystem* Renderer::CreateParticleSystem(u32 maxInitParticleCount)
{
  ParticleSystem* particleSystem = new ParticleSystem();
  particleSystem->Initialize(m_pRhi, m_particleEngine->GetParticleSystemDescriptorLayout(), maxInitParticleCount);
  particleSystem->PushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT | PARTICLE_DESCRIPTOR_UPDATE_BIT | PARTICLE_VERTEX_BUFFER_UPDATE_BIT);
  return particleSystem;
}


void Renderer::FreeParticleSystem(ParticleSystem* particle)
{
  if (!particle) { return; }
  particle->CleanUp(m_pRhi);
  delete particle;
}


UIDescriptor* Renderer::CreateUIDescriptor()
{
  UIDescriptor* pDesc = new UIDescriptor();
  return pDesc;
}


void Renderer::FreeUIDescriptor(UIDescriptor* pDesc)
{
  if (!pDesc) return;
  pDesc->CleanUp(m_pRhi);
  delete pDesc;
}
} // Recluse