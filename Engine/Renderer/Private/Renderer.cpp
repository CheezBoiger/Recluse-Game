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
  return Renderer::instance();
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
  , m_rhiBits(0)
  , m_globalLightProbe(nullptr)
  , m_pClusterer(nullptr)
  , m_particleEngine(nullptr)
  , m_usePreRenderSkybox(false)
  , m_pBakeIbl(nullptr)
  , m_pDebugManager(nullptr)
{
  m_HDR._Enabled = true;
  m_Downscale._Horizontal = 0;
  m_Downscale._Strength = 1.0f;
  m_Downscale._Scale = 1.0f;
  m_skybox._brdfLUT = nullptr;
  m_skybox._envmap = nullptr;
  m_skybox._irradiance = nullptr;
  m_skybox._specular = nullptr;


  m_cmdDeferredList.resize(1024);
  m_forwardCmdList.resize(1024);
  m_staticCmdList.resize(1024);
  m_dynamicCmdList.resize(1024);
  m_meshDescriptors.resize(1024);
  m_jointDescriptors.resize(1024);
  m_materialDescriptors.resize(1024);

  m_cmdDeferredList.setSortFunc([&](PrimitiveRenderCmd& cmd1, PrimitiveRenderCmd& cmd2) -> bool {
    
    //if (!cmd1._pTarget->_bRenderable || !cmd2._pTarget->_bRenderable) return false;
    MeshDescriptor* mesh1 = cmd1._pMeshDesc;
    MeshDescriptor* mesh2 = cmd2._pMeshDesc;

    if (!mesh1 || !mesh2) return false;
    Vector3 p1 = cmd1._pPrimitive->_aabb.centroid * mesh1->getObjectData()->_model;
    Vector3 p2 = cmd2._pPrimitive->_aabb.centroid * mesh2->getObjectData()->_model;
    Vector4 native_pos = m_pGlobal->getData()->_CameraPos;
    Vector3 cam_pos = Vector3(native_pos.x, native_pos.y, native_pos.z);
    Vector3 v1 = p1 - cam_pos;
    Vector3 v2 = p2 - cam_pos;
    r32 l1 = v1.length();
    r32 l2 = v2.length();
    R_ASSERT(!isnan(l1) && !isnan(l2), "");
    return (l1 < l2);
  });

  // Use painter's algorithm in this case for forward, simply because of 
  // transparent objects.
  m_forwardCmdList.setSortFunc([&](PrimitiveRenderCmd& cmd1, PrimitiveRenderCmd& cmd2) -> bool {
    //if (!cmd1._pTarget->_bRenderable || !cmd2._pTarget->_bRenderable) return false;
    MeshDescriptor* mesh1 = cmd1._pMeshDesc;
    MeshDescriptor* mesh2 = cmd2._pMeshDesc;
    if (!mesh1 || !mesh2) return false;
    Vector3 p1 = cmd1._pPrimitive->_aabb.centroid * mesh1->getObjectData()->_model;
    Vector3 p2 = cmd2._pPrimitive->_aabb.centroid *  mesh2->getObjectData()->_model;
    Vector4 native_pos = m_pGlobal->getData()->_CameraPos;
    Vector3 cam_pos = Vector3(native_pos.x, native_pos.y, native_pos.z);
    Vector3 v1 = p1 - cam_pos;
    Vector3 v2 = p2 - cam_pos;
    r32 l1 = v1.lengthSqr();
    r32 l2 = v2.lengthSqr();
    R_ASSERT(!isnan(l1) && !isnan(l2), "");
    //Log() << p1 << " l1: " << l1 << "\n" << p2 << " l2: " << l2 << "\n";
    return (l2 < l1);
  });
}


Renderer::~Renderer()
{
}

void Renderer::onStartUp()
{
  if (!gCore().isActive()) {
    R_DEBUG(rError, "Core is not active! Start up the core first!\n");
    return;
  }

  VulkanRHI::createContext(Renderer::appName);
  VulkanRHI::findPhysicalDevice(m_rhiBits);
  if (!m_pRhi) m_pRhi = new VulkanRHI();
  SetUpRenderData();
}


void Renderer::onShutDown()
{
  CleanUpRenderData();
  cleanUp();

  // Shutdown globals.
  VulkanRHI::gPhysicalDevice.cleanUp();
  VulkanRHI::gContext.cleanUp();
  R_DEBUG(rNotify, "Vulkan Renderer successfully cleaned up.\n");
}


void Renderer::beginFrame()
{
  m_Rendering = true;
  //m_pRhi->PresentWaitIdle();
  m_pRhi->acquireNextImage();
}


void Renderer::endFrame()
{
  m_Rendering = false;
  cleanStaticUpdate();
  if (m_pRhi->present() != VK_SUCCESS) {
    updateRendererConfigs(nullptr);
  }
}


void Renderer::waitForCpuFence()
{
  R_TIMED_PROFILE_RENDERER();

  VkFence fence[] = { m_cpuFence->getHandle() };
  m_pRhi->waitForFences(1, fence, VK_TRUE, UINT64_MAX);
  m_pRhi->resetFences(1, fence);
}


void Renderer::render()
{
  R_TIMED_PROFILE_RENDERER();

  if (m_Minimized) {
    // Window was minimized, ignore any cpu draw requests and prevent frame rendering
    // until window is back up.
    clearCmdLists();
    m_pUI->ClearUiBuffers();
    //WaitForCpuFence();
    return;
  }

  // TODO(): Signal a beginning and end callback or so, when performing 
  // any rendering.
  // Update the scene descriptors before rendering the frame.
  sortCmdLists();

  // Wait for fences before starting next frame.
  //WaitForCpuFence();
  m_pRhi->waitForFrameInFlightFence();

  // begin frame. This is where we start our render process per frame.
  beginFrame();
  u32 frameIndex = m_pRhi->currentFrame();
  updateSceneDescriptors(frameIndex);
  checkCmdUpdate();

  // TODO(): Need to clean this up.
  VkCommandBuffer offscreen_CmdBuffers[3] = { 
    m_Offscreen._cmdBuffers[frameIndex]->getHandle(), 
    nullptr,
    nullptr
  };

  VkSemaphore offscreen_WaitSemas[] = { m_pRhi->currentImageAvailableSemaphore() };
  VkSemaphore offscreen_SignalSemas[] = { m_Offscreen._semaphores[frameIndex]->getHandle() };
  VkSemaphore shadow_Signals[] = { m_Offscreen._shadowSemaphores[frameIndex]->getHandle() };
  VkSemaphore shadow_Waits[] = { m_Offscreen._shadowSemaphores[frameIndex]->getHandle(), m_Offscreen._semaphores[frameIndex]->getHandle() };

  VkCommandBuffer pbr_CmdBuffers[] = { m_Pbr._CmdBuffers[frameIndex]->getHandle() };
  VkSemaphore pbr_SignalSemas[] = { m_Pbr._Semas[frameIndex]->getHandle() };
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
  VkSemaphore skybox_SignalSemas[] = { m_SkyboxFinishedSignals[frameIndex]->getHandle() };
  VkCommandBuffer skybox_CmdBuffer = m_pSkyboxCmdBuffers[frameIndex]->getHandle();
  skyboxSI.commandBufferCount = 1;
  skyboxSI.pCommandBuffers = &skybox_CmdBuffer;
  skyboxSI.pSignalSemaphores = skybox_SignalSemas;
  skyboxSI.pWaitSemaphores = pbr_SignalSemas;

  VkSubmitInfo forwardSi = {};
  VkCommandBuffer forwardBuffers[] = { m_Forward._cmdBuffers[frameIndex]->getHandle() };
  VkSemaphore forwardSignalSemas[] = { m_Forward._semaphores[frameIndex]->getHandle() };
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
  VkSemaphore hdr_SignalSemas[] = { m_HDR._semaphores[frameIndex]->getHandle() };
  VkCommandBuffer hdrCmd = m_HDR._CmdBuffers[frameIndex]->getHandle();
  hdrSI.pCommandBuffers = &hdrCmd;
  hdrSI.pSignalSemaphores = hdr_SignalSemas;
  hdrSI.pWaitSemaphores = forwardSignalSemas;

  VkSemaphore* final_WaitSemas = hdr_SignalSemas;
  if (!m_HDR._Enabled) final_WaitSemas = forwardSignalSemas;

  VkSubmitInfo finalSi = { };
  finalSi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkCommandBuffer finalCmdBuffer = { m_pFinalCommandBuffers[frameIndex]->getHandle() };
  VkSemaphore finalFinished = { m_pFinalFinishedSemas[frameIndex]->getHandle() };
  finalSi.commandBufferCount = 1;
  finalSi.pCommandBuffers = &finalCmdBuffer;
  finalSi.pSignalSemaphores = &finalFinished;
  finalSi.pWaitDstStageMask = waitFlags;
  finalSi.signalSemaphoreCount = 1;
  finalSi.waitSemaphoreCount = 1;
  finalSi.pWaitSemaphores = final_WaitSemas;

  VkSubmitInfo uiSi = { };
  uiSi.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkCommandBuffer uiCmdBuffer = { m_pUI->GetCommandBuffer(frameIndex)->getHandle() };
  VkSemaphore uiSignalSema = { m_pUI->Signal(frameIndex)->getHandle() };
  uiSi.commandBufferCount = 1;
  uiSi.pCommandBuffers = &uiCmdBuffer;
  uiSi.waitSemaphoreCount = 1;
  uiSi.pWaitSemaphores = &finalFinished;
  uiSi.signalSemaphoreCount = 1;
  uiSi.pSignalSemaphores = &uiSignalSema;
  uiSi.pWaitDstStageMask = waitFlags;

  // Spinlock until we know this is finished.
  while (m_Offscreen._cmdBuffers[frameIndex]->recording()) {}

  // render shadow map here. Primary shadow map is our concern.
  if (m_pLights->isPrimaryShadowEnabled() || staticNeedsUpdate()) {
    R_DEBUG(rNotify, "Shadow.\n");
    u32 graphicsQueueCount = m_pRhi->graphicsQueueCount();
    if (graphicsQueueCount > 1) {

      VkSubmitInfo shadowSI = {};
      shadowSI.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      shadowSI.commandBufferCount = 1;
      VkCommandBuffer shadowCmdBuffer = m_Offscreen._shadowCmdBuffers[frameIndex]->getHandle();
      shadowSI.pCommandBuffers = &shadowCmdBuffer;
      shadowSI.pSignalSemaphores = shadow_Signals;
      shadowSI.pWaitDstStageMask = waitFlags;
      shadowSI.pWaitSemaphores = VK_NULL_HANDLE;
      shadowSI.signalSemaphoreCount = 1;
      shadowSI.waitSemaphoreCount = 0;
        
      pbrSi.waitSemaphoreCount = 2;
      pbrSi.pWaitSemaphores = shadow_Waits;
      m_pRhi->graphicsSubmit(1, 1, &shadowSI);
    } else {
      offscreen_CmdBuffers[offscreenSI.commandBufferCount] = m_Offscreen._shadowCmdBuffers[frameIndex]->getHandle();
      offscreenSI.commandBufferCount += 1; // Add shadow buffer to render.
    }
  }

  // Check if sky needs to update it's cubemap.
  if (m_pSky->NeedsRendering()) {
    offscreen_CmdBuffers[offscreenSI.commandBufferCount] = m_pSky->CmdBuffer()->getHandle();
    offscreenSI.commandBufferCount += 1; // Add sky render buffer.
    m_pSky->MarkClean();
  }

  VkSubmitInfo submits[] { offscreenSI, pbrSi, skyboxSI, forwardSi };
  // Submit to renderqueue.
  m_pRhi->graphicsSubmit(DEFAULT_QUEUE_IDX, 4, submits);

  //
  // TODO(): Add antialiasing here.
  // 

  // High Dynamic Range and getGamma Pass. Post process after rendering. This will include
  // getBloom, AA, other effects.
  if (m_HDR._Enabled) { m_pRhi->graphicsSubmit(DEFAULT_QUEUE_IDX, 1, &hdrSI); }

  // Final render after post process.
  m_pRhi->graphicsSubmit(DEFAULT_QUEUE_IDX, 1, &finalSi);

  // Before calling this cmd buffer, we want to submit our offscreen buffer first, then
  // sent our signal to our swapchain cmd buffers.
    
  // render the getOverlay.
  m_pRhi->graphicsSubmit(DEFAULT_QUEUE_IDX, 1, &uiSi);

  // Signal graphics finished on the final output.
  VkSemaphore signal = m_pRhi->currentGraphicsFinishedSemaphore();
  VkFence inflight = m_pRhi->currentInFlightFence();
  m_pRhi->submitCurrSwapchainCmdBuffer(1, &uiSignalSema, 1, &signal, inflight); // cpuFence will need to wait until overlay is finished.
  endFrame();

  // Compute pipeline render.
  VkSubmitInfo computeSubmit = { };
  computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  computeSubmit.commandBufferCount = 0;
  computeSubmit.pCommandBuffers = nullptr;
  computeSubmit.signalSemaphoreCount = 0;
  computeSubmit.waitSemaphoreCount = 0;

  m_pRhi->computeSubmit(DEFAULT_QUEUE_IDX, computeSubmit);

  // Clear command list afterwards.
  clearCmdLists();
}


void Renderer::cleanUp()
{
  // Must wait for all command buffers to finish before cleaning up.
  m_pRhi->deviceWaitIdle();

  m_pDebugManager->cleanUp(m_pRhi);
  delete m_pDebugManager;
  m_pDebugManager = nullptr;

  m_pAntiAliasingFXAA->cleanUp(m_pRhi);
  delete m_pAntiAliasingFXAA;
  m_pAntiAliasingFXAA = nullptr;

  m_pBakeIbl->cleanUp(m_pRhi);
  delete m_pBakeIbl;
  m_pBakeIbl = nullptr;

  m_pHDR->cleanUp(m_pRhi);
  delete m_pHDR;
  m_pHDR = nullptr;

  // We probably want to use smart ptrs...
  m_pGlobal->cleanUp(m_pRhi);
  delete m_pGlobal;
  m_pGlobal = nullptr;

  m_pLights->cleanUp(m_pRhi);
  delete m_pLights;
  m_pLights = nullptr;
  
  cleanUpSkybox(false);
  m_pSky->cleanUp();
  delete m_pSky;
  m_pSky = nullptr;

  if (m_pUI) {
    m_pUI->cleanUp(m_pRhi);
    delete m_pUI;
    m_pUI = nullptr;
  }

  if (m_particleEngine) {
    m_particleEngine->cleanUp(m_pRhi);
    delete m_particleEngine;
    m_particleEngine = nullptr;
  }

  cleanUpGlobalIlluminationBuffer();
  delete m_pGlobalIllumination;
  m_pGlobalIllumination = nullptr;

  m_RenderQuad.cleanUp(m_pRhi);
  cleanUpForwardPBR();
  cleanUpPBR();
  cleanUpHDR(true);
  cleanUpDownscale(true);
  cleanUpOffscreen();
  cleanUpFinalOutputs();
  cleanUpDescriptorSetLayouts();
  cleanUpGraphicsPipelines();
  ShadowMapSystem::cleanUpShadowPipelines(m_pRhi);
  cleanUpFrameBuffers();
  cleanUpRenderTextures(true);

  m_pRhi->freeVkFence(m_cpuFence);
  m_cpuFence = nullptr;

  if (m_pRhi) {
    m_pRhi->cleanUp();
    delete m_pRhi;
    m_pRhi = nullptr;
  }
  m_Initialized = false;
}


b32 Renderer::initialize(Window* window, const GraphicsConfigParams* params)
{
  if (!window) return false;
  if (m_Initialized) return true;

  if (!params) {
    params = &kDefaultGpuConfigs;
  }

  m_pWindow = window;

  m_renderWidth = (u32)m_pWindow->getWidth(); 
  m_renderHeight = (u32)m_pWindow->getHeight();

  updateRenderResolution(params->_Resolution);

  m_pRhi->initialize(window->getHandle(), params);
  {
    std::set<std::string> missing = VulkanRHI::getMissingExtensions(VulkanRHI::gPhysicalDevice.handle());
    
  }

  setUpRenderTextures(true);
  setUpFrameBuffers();
  setUpDescriptorSetLayouts();
  m_RenderQuad.initialize(m_pRhi); 

  GlobalDescriptor* gMat = new GlobalDescriptor();
  gMat->initialize(m_pRhi);
  gMat->getData()->_ScreenSize[0] = m_renderWidth;
  gMat->getData()->_ScreenSize[1] = m_renderHeight;
  for (u32 i = 0; i < m_pRhi->bufferingCount(); ++i) {
    gMat->update(m_pRhi, i);
  }
  m_pGlobal = gMat;

  m_pSky = new SkyRenderer();
  m_pSky->initialize();
  m_pSky->MarkDirty();

  m_pHDR = new HDR();
  m_pHDR->initialize(m_pRhi);
  m_pHDR->UpdateToGPU(m_pRhi);

  setUpSkybox(false);
  setUpGraphicsPipelines();

  // Dependency on shadow map pipeline initialization.
  ShadowMapSystem::initializeShadowPipelines(m_pRhi, params->_numberCascadeShadowMaps);
  m_pLights = new LightDescriptor();
  m_pLights->initialize(m_pRhi, params);

  for (u32 i = 0; i < m_pRhi->bufferingCount(); ++i) {
    m_pLights->update(m_pRhi, m_pGlobal->getData(), i);
  }

  updateRuntimeConfigs(params);
  m_pAntiAliasingFXAA = new AntiAliasingFXAA();
  m_pAntiAliasingFXAA->initialize(m_pRhi, m_pGlobal);

  setUpFinalOutputs();
  setUpOffscreen();
  setUpDownscale(true);
  setUpHDR(true);
  setUpPBR();
  setUpForwardPBR();

  m_pBakeIbl = new BakeIBL();
  m_pBakeIbl->initialize(m_pRhi);

  m_particleEngine = new ParticleEngine();
  m_particleEngine->initialize(m_pRhi);

  m_pDebugManager = new DebugManager();
  m_pDebugManager->initialize(m_pRhi);

  {
    u32 vendorId = m_pRhi->vendorID();
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
  
  m_cpuFence = m_pRhi->createVkFence();
  m_cpuFence->initialize(fenceCi);

  m_pRhi->setSwapchainCmdBufferBuild([&] (CommandBuffer& cmdBuffer, VkRenderPassBeginInfo& defaultRenderpass) -> void {
    // Do stuff with the buffer.
    VkExtent2D windowExtent = { m_pWindow->getWidth(), m_pWindow->getHeight() };
    VkViewport viewport = { };
    viewport.height = (r32) windowExtent.height;
    viewport.width = (r32) windowExtent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = 0.0f;
    viewport.y = 0.0f;

    VkRect2D scissor = { };
    scissor.offset = { 0, 0 };
    scissor.extent = windowExtent;

    GraphicsPipeline* finalPipeline = output_pipelineKey;
    DescriptorSet* finalSet = output_descSetKey;
    
    cmdBuffer.beginRenderPass(defaultRenderpass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer.setViewPorts(0, 1, &viewport);
      cmdBuffer.setScissor(0, 1, &scissor);
      cmdBuffer.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Pipeline());
      VkDescriptorSet finalDescriptorSets[] = { finalSet->getHandle() };    

      cmdBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->getLayout(), 0, 1, finalDescriptorSets, 0, nullptr);
      VkBuffer vertexBuffer = m_RenderQuad.getQuad()->getHandle()->getNativeBuffer();
      VkBuffer indexBuffer = m_RenderQuad.getIndices()->getHandle()->getNativeBuffer();
      VkDeviceSize offsets[] = { 0 };

      cmdBuffer.bindIndexBuffer(indexBuffer, 0, getNativeIndexType(m_RenderQuad.getIndices()->GetSizeType()));
      cmdBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

      cmdBuffer.drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer.endRenderPass();
  });

  if (!m_pUI) {
    m_pUI = new UIOverlay();
    m_pUI->initialize(m_pRhi);
  }

  m_pGlobalIllumination = new GlobalIllumination();

  setUpGlobalIlluminationBuffer();

  updateGlobalIlluminationBuffer();

  m_Initialized = true;
  return true;
}


void Renderer::setUpDescriptorSetLayouts()
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

    DescriptorSetLayout* LightViewLayout = m_pRhi->createDescriptorSetLayout();
  
    VkDescriptorSetLayoutCreateInfo LightViewInfo = { };
    LightViewInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LightViewInfo.bindingCount = static_cast<u32>(LightViewBindings.size());
    LightViewInfo.pBindings = LightViewBindings.data();
    LightViewLayout->initialize(LightViewInfo);

    LightViewDescriptorSetLayoutKey = LightViewLayout;  
  }

  DescriptorSetLayout* GlobalSetLayout = m_pRhi->createDescriptorSetLayout();
  DescriptorSetLayout* MeshSetLayout = m_pRhi->createDescriptorSetLayout();
  DescriptorSetLayout* MaterialSetLayout = m_pRhi->createDescriptorSetLayout();
  DescriptorSetLayout* LightSetLayout = m_pRhi->createDescriptorSetLayout();
  DescriptorSetLayout* BonesSetLayout = m_pRhi->createDescriptorSetLayout();
  DescriptorSetLayout* SkySetLayout = m_pRhi->createDescriptorSetLayout();
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
    GlobalSetLayout->initialize(GlobalLayout);
    MeshSetLayout->initialize(GlobalLayout);
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

    MaterialSetLayout->initialize(MaterialLayout);
  }

  // PBR descriptor layout.
  {
    DescriptorSetLayout* pbr_Layout = m_pRhi->createDescriptorSetLayout();
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

    // _position
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
    pbr_Layout->initialize(PbrLayout);
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
    BonesSetLayout->initialize(BoneLayout);
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
    SkySetLayout->initialize(info);
  }

  // Global Illumination reflection probe layout.
  {
    globalIllumination_DescNoLR = m_pRhi->createDescriptorSetLayout();
    globalIllumination_DescLR = m_pRhi->createDescriptorSetLayout();
    
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
    globalIllumination_DescLR->initialize(info);
    info.bindingCount = static_cast<u32>(globalIllum.size() - 3);
    globalIllumination_DescNoLR->initialize(info);
  }

  // Compute PBR textures.
  {
    pbr_compDescLayout = m_pRhi->createDescriptorSetLayout();
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

    pbr_compDescLayout->initialize(info);
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

    LightSetLayout->initialize(LightLayout);
  }
  // Final/Output Layout pass.
  {
    DescriptorSetLayout* finalSetLayout = m_pRhi->createDescriptorSetLayout();
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

    finalSetLayout->initialize(finalLayoutInfo);
  }
  // HDR Layout pass.
  DescriptorSetLayout* hdrSetLayout = m_pRhi->createDescriptorSetLayout();
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
  
  hdrSetLayout->initialize(hdrLayoutCi);

  // Downscale descriptor set layout info.
  DescriptorSetLayout* downscaleLayout = m_pRhi->createDescriptorSetLayout();
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
  
  downscaleLayout->initialize(dwnLayout);

  DescriptorSetLayout* GlowLayout = m_pRhi->createDescriptorSetLayout();
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

  GlowLayout->initialize(dwnLayout);
}


void Renderer::cleanUpDescriptorSetLayouts()
{
  m_pRhi->freeDescriptorSetLayout(GlobalSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(MeshSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(MaterialSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(LightSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(BonesSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(skybox_setLayoutKey);
  m_pRhi->freeDescriptorSetLayout(LightViewDescriptorSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(final_DescSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(hdr_gamma_descSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(DownscaleBlurLayoutKey);
  m_pRhi->freeDescriptorSetLayout(GlowDescriptorSetLayoutKey);
  m_pRhi->freeDescriptorSetLayout(pbr_DescLayoutKey);
  m_pRhi->freeDescriptorSetLayout(pbr_compDescLayout);
  m_pRhi->freeDescriptorSetLayout(globalIllumination_DescLR);
  m_pRhi->freeDescriptorSetLayout(globalIllumination_DescNoLR);
}


void Renderer::setUpFrameBuffers()
{
  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };
  // m_pRhi->swapchainObject()->SwapchainExtent();
  Texture* gbuffer_Albedo = gbuffer_AlbedoAttachKey;
  Texture* gbuffer_Normal = gbuffer_NormalAttachKey;
  Texture* gbuffer_Position = gbuffer_PositionAttachKey;
  Texture* gbuffer_Emission = gbuffer_EmissionAttachKey;
  Texture* gbuffer_Depth = gbuffer_DepthAttachKey;

  FrameBuffer* gbuffer_FrameBuffer = m_pRhi->createFrameBuffer();
  gbuffer_FrameBufferKey = gbuffer_FrameBuffer;

  FrameBuffer* pbr_FrameBuffer = m_pRhi->createFrameBuffer();
  pbr_FrameBufferKey = pbr_FrameBuffer;

  FrameBuffer* hdrFrameBuffer = m_pRhi->createFrameBuffer();
  hdr_gamma_frameBufferKey =  hdrFrameBuffer;

  final_frameBufferKey = m_pRhi->createFrameBuffer();

  // Final framebuffer.
  {
    std::array<VkAttachmentDescription, 1> attachmentDescriptions;
    VkSubpassDependency dependencies[2];
    attachmentDescriptions[0] = CreateAttachmentDescription(
      final_renderTargetKey->getFormat(),
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      final_renderTargetKey->getSamples()
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
    attachments[0] = final_renderTargetKey->getView();
    VkFramebufferCreateInfo framebufferCI = CreateFrameBufferInfo(
      windowExtent.width,
      windowExtent.height,
      nullptr, // Finalize() call handles this for us.
      static_cast<u32>(attachments.size()),
      attachments.data(),
      1
    );
    final_renderPass = m_pRhi->createRenderPass();
    final_renderPass->initialize(renderpassCI);
    final_frameBufferKey->Finalize(framebufferCI, final_renderPass);
  }

  std::array<VkAttachmentDescription, 5> attachmentDescriptions;
  VkSubpassDependency dependencies[2];

  attachmentDescriptions[0] = CreateAttachmentDescription(
    gbuffer_Albedo->getFormat(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Albedo->getSamples()
  );

  attachmentDescriptions[1] = CreateAttachmentDescription(
    gbuffer_Normal->getFormat(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Normal->getSamples()
  );

  attachmentDescriptions[2] = CreateAttachmentDescription(
    gbuffer_Position->getFormat(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Position->getSamples()
  );

  attachmentDescriptions[3] = CreateAttachmentDescription(
    gbuffer_Emission->getFormat(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    gbuffer_Emission->getSamples()
  );

  attachmentDescriptions[4] = CreateAttachmentDescription(
    gbuffer_Depth->getFormat(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    gbuffer_Depth->getSamples()
  );

  dependencies[0] = CreateSubPassDependency(
    VK_SUBPASS_EXTERNAL, 
    VK_ACCESS_MEMORY_WRITE_BIT, 
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    0, 
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 
    VK_DEPENDENCY_BY_REGION_BIT
  );

  dependencies[1] = CreateSubPassDependency(
    0,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_SUBPASS_EXTERNAL,
    VK_ACCESS_MEMORY_WRITE_BIT,
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
  attachments[0] = gbuffer_Albedo->getView();
  attachments[1] = gbuffer_Normal->getView();
  attachments[2] = gbuffer_Position->getView();
  attachments[3] = gbuffer_Emission->getView();
  attachments[4] = gbuffer_Depth->getView();

  VkFramebufferCreateInfo framebufferCI = CreateFrameBufferInfo(
    windowExtent.width,
    windowExtent.height,
    nullptr, // Finalize() call handles this for us.
    static_cast<u32>(attachments.size()),
    attachments.data(),
    1
  );

  gbuffer_renderPass = m_pRhi->createRenderPass();
  gbuffer_renderPass->initialize(renderpassCI);
  gbuffer_FrameBuffer->Finalize(framebufferCI, gbuffer_renderPass);

  // pbr framebuffer.
  {
    Texture* pbr_Bright = pbr_BrightTextureKey;
    Texture* pbr_Final = pbr_FinalTextureKey;
    std::array<VkAttachmentDescription, 7> pbrAttachmentDescriptions;
    pbrAttachmentDescriptions[0] = CreateAttachmentDescription(
      pbr_Final->getFormat(),
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
     pbr_Final->getSamples()
    );

    pbrAttachmentDescriptions[1] = CreateAttachmentDescription(
      pbr_Bright->getFormat(),
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      pbr_Bright->getSamples()
    );

    pbrAttachmentDescriptions[2] = CreateAttachmentDescription(
      gbuffer_Depth->getFormat(),
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      gbuffer_Depth->getSamples()
    );

    VkSubpassDescription pbrSubpass = {};
    pbrSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    pbrSubpass.colorAttachmentCount = static_cast<u32>(pbrAttachmentDescriptions.size() - 5);
    pbrSubpass.pColorAttachments = attachmentColors.data();
    attachmentDepthRef.attachment = 2;
    attachmentDepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
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
    pbrAttachments[0] = pbr_Final->getView();
    pbrAttachments[1] = pbr_Bright->getView();
    pbrAttachments[2] = gbuffer_Depth->getView();

    VkFramebufferCreateInfo pbrFramebufferCI = CreateFrameBufferInfo(
      windowExtent.width,
      windowExtent.height,
      nullptr, // Finalize() call handles this for us.
      static_cast<u32>(pbrAttachments.size() - 4),
      pbrAttachments.data(),
      1
    );

    pbr_renderPass = m_pRhi->createRenderPass();
    pbr_renderPass->initialize(pbrRenderpassCI);
    pbr_FrameBuffer->Finalize(pbrFramebufferCI, pbr_renderPass);

    // Forward renderpass portion.
    pbrAttachmentDescriptions[0] = CreateAttachmentDescription(
      pbr_Final->getFormat(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      pbr_Final->getSamples()
    );

    pbrAttachmentDescriptions[1] = CreateAttachmentDescription(
      pbr_Bright->getFormat(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      pbr_Bright->getSamples()
    );

    pbrAttachmentDescriptions[2] = CreateAttachmentDescription(
      gbuffer_Albedo->getFormat(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Albedo->getSamples()
    );

    pbrAttachmentDescriptions[3] = CreateAttachmentDescription(
      gbuffer_Normal->getFormat(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Normal->getSamples()
    );

    pbrAttachmentDescriptions[4] = CreateAttachmentDescription(
      gbuffer_Position->getFormat(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Position->getSamples()
    );

    pbrAttachmentDescriptions[5] = CreateAttachmentDescription(
      gbuffer_Emission->getFormat(),
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Emission->getSamples()
    );

    pbrAttachmentDescriptions[6] = CreateAttachmentDescription(
      gbuffer_Depth->getFormat(),
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
      VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      gbuffer_Depth->getSamples()
    );

    pbrAttachments[0] = pbr_Final->getView();
    pbrAttachments[1] = pbr_Bright->getView();
    pbrAttachments[2] = gbuffer_Albedo->getView();
    pbrAttachments[3] = gbuffer_Normal->getView();
    pbrAttachments[4] = gbuffer_Position->getView();
    pbrAttachments[5] = gbuffer_Emission->getView();
    pbrAttachments[6] = gbuffer_Depth->getView();

    pbrSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    pbrSubpass.colorAttachmentCount = static_cast<u32>(pbrAttachmentDescriptions.size() - 1);
    pbrSubpass.pColorAttachments = attachmentColors.data();
    attachmentDepthRef.attachment = 6;
    attachmentDepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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

    pbr_forwardRenderPass = m_pRhi->createRenderPass();
    pbr_forwardFrameBuffer = m_pRhi->createFrameBuffer();
    pbr_forwardRenderPass->initialize(pbrRenderpassCI);
    pbr_forwardFrameBuffer->Finalize(pbrFramebufferCI, pbr_forwardRenderPass);
    
  }
  
  // No need to render any depth, as we are only writing on a 2d surface.
  Texture* hdrColor = hdr_gamma_colorAttachKey;
  subpass.pDepthStencilAttachment = nullptr;
  attachments[0] = hdrColor->getView();
  framebufferCI.attachmentCount = 1;
  attachmentDescriptions[0].format = hdrColor->getFormat();
  attachmentDescriptions[0].samples = hdrColor->getSamples();
  attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  renderpassCI.attachmentCount = 1;
  subpass.colorAttachmentCount = 1;
  hdr_renderPass = m_pRhi->createRenderPass();
  hdr_renderPass->initialize(renderpassCI);
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

  FrameBuffer* DownScaleFB2x = m_pRhi->createFrameBuffer();
  FrameBuffer* FB2xFinal = m_pRhi->createFrameBuffer();
  FrameBuffer* DownScaleFB4x = m_pRhi->createFrameBuffer();
  FrameBuffer* FB4xFinal = m_pRhi->createFrameBuffer();
  FrameBuffer* DownScaleFB8x = m_pRhi->createFrameBuffer();
  FrameBuffer* FB8xFinal = m_pRhi->createFrameBuffer();
  FrameBuffer* DownScaleFB16x = m_pRhi->createFrameBuffer();
  FrameBuffer* FB16xFinal = m_pRhi->createFrameBuffer();
  FrameBuffer* GlowFB = m_pRhi->createFrameBuffer();
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
  attachments[0] = RenderTarget2xFinal->getView();
  attachmentDescriptions[0].format = RenderTarget2xFinal->getFormat();
  attachmentDescriptions[0].samples = RenderTarget2xFinal->getSamples();
  framebufferCI.width = RenderTarget2xFinal->getWidth();
  framebufferCI.height = RenderTarget2xFinal->getHeight();
  FB2xFinal->Finalize(framebufferCI, hdr_renderPass);

  attachments[0] = rtDownScale2x->getView();
  attachmentDescriptions[0].format = rtDownScale2x->getFormat();
  attachmentDescriptions[0].samples = rtDownScale2x->getSamples();
  framebufferCI.width = rtDownScale2x->getWidth();
  framebufferCI.height = rtDownScale2x->getHeight();
  DownScaleFB2x->Finalize(framebufferCI, hdr_renderPass);

  // 4x
  attachments[0] = RenderTarget4xFinal->getView();
  attachmentDescriptions[0].format = RenderTarget4xFinal->getFormat();
  attachmentDescriptions[0].samples = RenderTarget4xFinal->getSamples();
  framebufferCI.width = RenderTarget4xFinal->getWidth();
  framebufferCI.height = RenderTarget4xFinal->getHeight();
  FB4xFinal->Finalize(framebufferCI, hdr_renderPass);

  attachments[0] = rtDownScale4x->getView();
  attachmentDescriptions[0].format = rtDownScale4x->getFormat();
  attachmentDescriptions[0].samples = rtDownScale4x->getSamples();
  framebufferCI.width = rtDownScale4x->getWidth();
  framebufferCI.height = rtDownScale4x->getHeight();
  DownScaleFB4x->Finalize(framebufferCI, hdr_renderPass);

  // 8x
  attachments[0] = RenderTarget8xFinal->getView();
  attachmentDescriptions[0].format = RenderTarget8xFinal->getFormat();
  attachmentDescriptions[0].samples = RenderTarget8xFinal->getSamples();
  framebufferCI.width = RenderTarget8xFinal->getWidth();
  framebufferCI.height = RenderTarget8xFinal->getHeight();
  FB8xFinal->Finalize(framebufferCI, hdr_renderPass);

  attachments[0] = rtDownScale8x->getView();
  attachmentDescriptions[0].format = rtDownScale8x->getFormat();
  attachmentDescriptions[0].samples = rtDownScale8x->getSamples();
  framebufferCI.width = rtDownScale8x->getWidth();
  framebufferCI.height = rtDownScale8x->getHeight();
  DownScaleFB8x->Finalize(framebufferCI, hdr_renderPass);

  // 16x
  attachments[0] = RenderTarget16xFinal->getView();
  attachmentDescriptions[0].format = RenderTarget16xFinal->getFormat();
  attachmentDescriptions[0].samples = RenderTarget16xFinal->getSamples();
  framebufferCI.width = RenderTarget16xFinal->getWidth();
  framebufferCI.height = RenderTarget16xFinal->getHeight();
  FB16xFinal->Finalize(framebufferCI, hdr_renderPass);

  attachments[0] = rtDownScale16x->getView();
  attachmentDescriptions[0].format = rtDownScale16x->getFormat();
  attachmentDescriptions[0].samples = rtDownScale16x->getSamples();
  framebufferCI.width = rtDownScale16x->getWidth();
  framebufferCI.height = rtDownScale16x->getHeight();
  DownScaleFB16x->Finalize(framebufferCI, hdr_renderPass);

  // Glow
  attachments[0] = GlowTarget->getView();
  attachmentDescriptions[0].format = GlowTarget->getFormat();
  attachmentDescriptions[0].samples = GlowTarget->getSamples();
  framebufferCI.width = m_renderWidth;
  framebufferCI.height = m_renderHeight;
  GlowFB->Finalize(framebufferCI, hdr_renderPass);
}


void Renderer::setUpGraphicsPipelines()
{
  RendererPass::initialize(m_pRhi);

  VkPipelineInputAssemblyStateCreateInfo assemblyCI = { };
  assemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  assemblyCI.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = { };
  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.height = static_cast<r32>(windowExtent.height);
  viewport.width = static_cast<r32>(windowExtent.width);

  VkRect2D scissor = { };
  scissor.extent = windowExtent;
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
  depthStencilCI.depthBoundsTestEnable = m_pRhi->depthBoundsAllowed();
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
    
  RendererPass::SetUpForwardPhysicallyBasedPass(getRHI(), GraphicsPipelineInfo);

  RendererPass::SetUpGBufferPass(getRHI(), GraphicsPipelineInfo);
  RendererPass::SetUpSkyboxPass(getRHI(), GraphicsPipelineInfo);

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

  RendererPass::SetUpDeferredPhysicallyBasedPass(getRHI(), GraphicsPipelineInfo);
  RendererPass::setUpDownScalePass(getRHI(), GraphicsPipelineInfo);
  RendererPass::SetUpHDRGammaPass(getRHI(), GraphicsPipelineInfo, m_pHDR);

  if (m_AntiAliasing) {
    RendererPass::setUpAAPass(getRHI(), GraphicsPipelineInfo, m_currentGraphicsConfigs._AA);
  }

  colorBlendAttachments[0].blendEnable = VK_FALSE;
  RendererPass::SetUpFinalPass(getRHI(), GraphicsPipelineInfo);
}


void Renderer::cleanUpGraphicsPipelines()
{

  RendererPass::cleanUp(m_pRhi);

  m_pRhi->freeGraphicsPipeline(pbr_forwardPipeline_LR);
  m_pRhi->freeGraphicsPipeline(pbr_staticForwardPipeline_LR);
  m_pRhi->freeGraphicsPipeline(pbr_forwardPipeline_NoLR);
  m_pRhi->freeGraphicsPipeline(pbr_staticForwardPipeline_NoLR);
  m_pRhi->freeGraphicsPipeline(transparent_staticShadowPipe);
  m_pRhi->freeGraphicsPipeline(transparent_dynamicShadowPipe);
  m_pRhi->freeGraphicsPipeline(transparent_colorFilterPipe);
  m_pRhi->freeGraphicsPipeline(pbr_forwardPipelineMorphTargets_LR);
  m_pRhi->freeGraphicsPipeline(pbr_forwardPipelineMorphTargets_NoLR);
  m_pRhi->freeGraphicsPipeline(pbr_staticForwardPipelineMorphTargets_LR);
  m_pRhi->freeGraphicsPipeline(pbr_staticForwardPipelineMorphTargets_NoLR);
  m_pRhi->freeGraphicsPipeline(pbr_static_LR_Debug);
  m_pRhi->freeGraphicsPipeline(pbr_static_NoLR_Debug);
  m_pRhi->freeGraphicsPipeline(pbr_static_mt_LR_Debug);
  m_pRhi->freeGraphicsPipeline(pbr_static_mt_NoLR_Debug);
  m_pRhi->freeGraphicsPipeline(pbr_dynamic_LR_Debug);
  m_pRhi->freeGraphicsPipeline(pbr_dynamic_LR_mt_Debug);
  m_pRhi->freeGraphicsPipeline(pbr_dynamic_NoLR_Debug);
  m_pRhi->freeGraphicsPipeline(pbr_dynamic_NoLR_mt_Debug);

  GraphicsPipeline* QuadPipeline = final_PipelineKey;
  m_pRhi->freeGraphicsPipeline(QuadPipeline);

  m_pRhi->freeGraphicsPipeline(output_pipelineKey);
  m_pRhi->freeComputePipeline(pbr_computePipeline_NoLR);
  m_pRhi->freeComputePipeline(pbr_computePipeline_LR);

  GraphicsPipeline* DownscalePipeline2x = DownscaleBlurPipeline2xKey;
  m_pRhi->freeGraphicsPipeline(DownscalePipeline2x);

  GraphicsPipeline* DownscalePipeline4x = DownscaleBlurPipeline4xKey;
  m_pRhi->freeGraphicsPipeline(DownscalePipeline4x);

  GraphicsPipeline* DownscalePipeline8x = DownscaleBlurPipeline8xKey;
  m_pRhi->freeGraphicsPipeline(DownscalePipeline8x);

  GraphicsPipeline* DownscalePipeline16x = DownscaleBlurPipeline16xKey;
  m_pRhi->freeGraphicsPipeline(DownscalePipeline16x);

  GraphicsPipeline* GlowPipeline = GlowPipelineKey;
  m_pRhi->freeGraphicsPipeline(GlowPipeline);

  GraphicsPipeline* SkyPipeline = skybox_pipelineKey;
  m_pRhi->freeGraphicsPipeline(SkyPipeline);

  GraphicsPipeline* ShadowMapPipeline = ShadowMapPipelineKey;
  GraphicsPipeline* DynamicShadowMapPipline = DynamicShadowMapPipelineKey;
  if (ShadowMapPipeline) {
    m_pRhi->freeGraphicsPipeline(ShadowMapPipeline);
  }
  
  if (DynamicShadowMapPipline) {
    m_pRhi->freeGraphicsPipeline(DynamicShadowMapPipline);
  }
}


void Renderer::cleanUpFrameBuffers()
{
  FrameBuffer* gbuffer_FrameBuffer = gbuffer_FrameBufferKey;
  m_pRhi->freeRenderPass(gbuffer_renderPass);
  m_pRhi->freeFrameBuffer(gbuffer_FrameBuffer);

  FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;
  m_pRhi->freeRenderPass(pbr_renderPass);
  m_pRhi->freeFrameBuffer(pbr_FrameBuffer);

  FrameBuffer* hdrFrameBuffer = hdr_gamma_frameBufferKey;
  m_pRhi->freeRenderPass(hdr_renderPass);
  m_pRhi->freeFrameBuffer(hdrFrameBuffer);

  FrameBuffer* DownScaleFB2x = FrameBuffer2xHorizKey;
  FrameBuffer* FB2xFinal = FrameBuffer2xFinalKey;
  FrameBuffer* DownScaleFB4x = FrameBuffer4xKey;
  FrameBuffer* FB4xFinal = FrameBuffer4xFinalKey;
  FrameBuffer* DownScaleFB8x = FrameBuffer8xKey;
  FrameBuffer* FB8xFinal = FrameBuffer8xFinalKey;
  FrameBuffer* DownScaleFB16x = FrameBuffer16xKey;
  FrameBuffer* FB16xFinal = FrameBuffer16xFinalKey;
  FrameBuffer* GlowFB = FrameBufferGlowKey;

  m_pRhi->freeFrameBuffer(DownScaleFB2x);
  m_pRhi->freeFrameBuffer(DownScaleFB4x);
  m_pRhi->freeFrameBuffer(DownScaleFB8x);
  m_pRhi->freeFrameBuffer(DownScaleFB16x);
  m_pRhi->freeFrameBuffer(FB2xFinal);
  m_pRhi->freeFrameBuffer(FB4xFinal);
  m_pRhi->freeFrameBuffer(FB8xFinal);
  m_pRhi->freeFrameBuffer(FB16xFinal);
  m_pRhi->freeFrameBuffer(GlowFB);
  
  m_pRhi->freeRenderPass(final_renderPass);
  m_pRhi->freeFrameBuffer(pbr_forwardFrameBuffer);
  m_pRhi->freeFrameBuffer(final_frameBufferKey);

  m_pRhi->freeRenderPass(pbr_forwardRenderPass);
}


void Renderer::setUpRenderTextures(b32 fullSetup)
{
  Texture* renderTarget2xScaled = m_pRhi->createTexture();
  Texture* RenderTarget2xFinal = m_pRhi->createTexture();
  Texture* renderTarget4xScaled = m_pRhi->createTexture();
  Texture* RenderTarget4xFinal = m_pRhi->createTexture();
  Texture* renderTarget8xScaled = m_pRhi->createTexture();
  Texture* RenderTarget8xFinal = m_pRhi->createTexture();
  Texture* RenderTarget16xScaled = m_pRhi->createTexture();
  Texture* RenderTarget16xFinal = m_pRhi->createTexture();

  Texture* pbr_Final = m_pRhi->createTexture();
  Texture* pbr_Bright = m_pRhi->createTexture();
  Texture* GlowTarget = m_pRhi->createTexture();

  Texture* gbuffer_Albedo = m_pRhi->createTexture();
  Texture* gbuffer_Normal = m_pRhi->createTexture();
  Texture* gbuffer_roughMetalSpec = m_pRhi->createTexture();
  Texture* gbuffer_Emission = m_pRhi->createTexture();
  Texture* gbuffer_Depth = m_pRhi->createTexture();
  Sampler* gbuffer_Sampler = m_pRhi->createSampler();

  RDEBUG_SET_VULKAN_NAME(gbuffer_Albedo, "Albedo");
  RDEBUG_SET_VULKAN_NAME(gbuffer_Normal, "Normal");
  RDEBUG_SET_VULKAN_NAME(gbuffer_roughMetalSpec, "RoughMetal");
  RDEBUG_SET_VULKAN_NAME(gbuffer_Depth, "Depth");
  RDEBUG_SET_VULKAN_NAME(gbuffer_Emission, "Emissive");

  RDEBUG_SET_VULKAN_NAME(pbr_Final, "PBR Lighting");
  RDEBUG_SET_VULKAN_NAME(pbr_Bright, "Bright");
  RDEBUG_SET_VULKAN_NAME(GlowTarget, "Glow");

  Texture* hdr_Texture = m_pRhi->createTexture();
  Sampler* hdr_Sampler = m_pRhi->createSampler();
  RDEBUG_SET_VULKAN_NAME(hdr_Texture, "HDR");
  
  Texture* final_renderTexture = m_pRhi->createTexture();

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
  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };

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
  cViewInfo.image = nullptr; // No need to set the image, texture->initialize() handles this for us.
  cViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  cViewInfo.subresourceRange = { };
  cViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  cViewInfo.subresourceRange.baseArrayLayer = 0;
  cViewInfo.subresourceRange.baseMipLevel = 0;
  cViewInfo.subresourceRange.layerCount = 1;
  cViewInfo.subresourceRange.levelCount = 1;

  gbuffer_Albedo->initialize(cImageInfo, cViewInfo);  

  cImageInfo.format = GBUFFER_ADDITIONAL_INFO_FORMAT;
  cViewInfo.format =  GBUFFER_ADDITIONAL_INFO_FORMAT;
  gbuffer_Emission->initialize(cImageInfo, cViewInfo);

  cImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  pbr_Final->initialize(cImageInfo, cViewInfo);
  final_renderTexture->initialize(cImageInfo, cViewInfo);
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  cImageInfo.format = GBUFFER_ROUGH_METAL_FORMAT;
  cViewInfo.format = GBUFFER_ROUGH_METAL_FORMAT;
  gbuffer_roughMetalSpec->initialize(cImageInfo, cViewInfo);

  // For the normal component in the gbuffer we use 10 bits for r, g, and b channels, and 2 bits for alpha.
  // If this is not supported properly, we can go with less precision.
  VkImageFormatProperties imgFmtProps;
  VkResult result = VulkanRHI::gPhysicalDevice.getImageFormatProperties(VK_FORMAT_R16G16_UNORM, 
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
  gbuffer_Normal->initialize(cImageInfo, cViewInfo);

  // TODO(): Need to replace position render target, as we can take advantage of 
  // depth buffer and clip space fragment coordinates.
  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  pbr_Bright->initialize(cImageInfo, cViewInfo);
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  GlowTarget->initialize(cImageInfo, cViewInfo);

  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  // Initialize downscaled render textures.
  cImageInfo.extent.width = windowExtent.width    >> 1;
  cImageInfo.extent.height = windowExtent.height  >> 1;
  renderTarget2xScaled->initialize(cImageInfo, cViewInfo);
  RenderTarget2xFinal->initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = windowExtent.width    >> 2;
  cImageInfo.extent.height = windowExtent.height  >> 2;
  renderTarget4xScaled->initialize(cImageInfo, cViewInfo);
  RenderTarget4xFinal->initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = windowExtent.width    >> 3;
  cImageInfo.extent.height = windowExtent.height  >> 3;
  renderTarget8xScaled->initialize(cImageInfo, cViewInfo);
  RenderTarget8xFinal->initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = windowExtent.width    >> 4;
  cImageInfo.extent.height = windowExtent.height  >> 4;
  RenderTarget16xScaled->initialize(cImageInfo, cViewInfo);
  RenderTarget16xFinal->initialize(cImageInfo, cViewInfo);

  // Depth attachment texture.
  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.extent.width = windowExtent.width;
  cImageInfo.extent.height = windowExtent.height;
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  hdr_Texture->initialize(cImageInfo, cViewInfo);

  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.extent.width = windowExtent.width;
  cImageInfo.extent.height = windowExtent.height;
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.usage = m_pRhi->depthUsageFlags() | VK_IMAGE_USAGE_SAMPLED_BIT;
  cImageInfo.format = m_pRhi->depthFormat();

  cViewInfo.format = m_pRhi->depthFormat();
  cViewInfo.subresourceRange.aspectMask = m_pRhi->depthAspectFlags();

  gbuffer_Depth->initialize(cImageInfo, cViewInfo);

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

  gbuffer_Sampler->initialize(samplerCI);
  hdr_Sampler->initialize(samplerCI);

  if (fullSetup) {
    Sampler* defaultSampler = m_pRhi->createSampler();
    defaultSampler->initialize(samplerCI);
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

    Texture* defaultTexture = m_pRhi->createTexture();
    RDEBUG_SET_VULKAN_NAME(defaultTexture, "Default Texture");
    defaultTexture->initialize(dImageInfo, dViewInfo);
    DefaultTextureKey = defaultTexture;

    {
      Texture2D tex2d;
      tex2d.mRhi = m_pRhi;
      tex2d.texture = defaultTexture;
      tex2d.m_bGenMips = false;
      Image img; img._data = new u8[4]; img._height = 1; img._width = 1; img._memorySize = 4;
      tex2d.update(img);
      delete img._data;
    }

    if (!DefaultTexture2DArrayView) {
      dViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
      dViewInfo.image = defaultTexture->getImage();
      vkCreateImageView(m_pRhi->logicDevice()->getNative(), &dViewInfo, nullptr, &DefaultTexture2DArrayView);
    }
  }
}


void Renderer::cleanUpRenderTextures(b32 fullCleanup)
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

    m_pRhi->freeTexture(renderTarget2xScaled);
    m_pRhi->freeTexture(RenderTarget2xFinal);
    m_pRhi->freeTexture(renderTarget4xScaled);
    m_pRhi->freeTexture(RenderTarget4xFinal);
    m_pRhi->freeTexture(RenderTarget8xFinal);
    m_pRhi->freeTexture(renderTarget8xScaled);
    m_pRhi->freeTexture(RenderTarget16xScaled);
    m_pRhi->freeTexture(RenderTarget16xFinal);
    m_pRhi->freeTexture(GlowTarget);
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
  
    m_pRhi->freeTexture(hdr_Texture);
    m_pRhi->freeSampler(hdr_Sampler);

    m_pRhi->freeTexture(gbuffer_Albedo);
    m_pRhi->freeTexture(gbuffer_Normal);
    m_pRhi->freeTexture(gbuffer_Position);
    m_pRhi->freeTexture(gbuffer_Emission);
    m_pRhi->freeTexture(gbuffer_Depth);
    m_pRhi->freeSampler(gbuffer_Sampler);
    m_pRhi->freeTexture(pbr_Bright);
    m_pRhi->freeTexture(pbr_Final);
    m_pRhi->freeTexture(final_renderTarget);
  }
  if (fullCleanup) {
    Texture* defaultTexture = DefaultTextureKey;
    Sampler* defaultSampler = DefaultSampler2DKey;

    m_pRhi->freeTexture(defaultTexture);
    m_pRhi->freeSampler(defaultSampler);

    if (DefaultTexture2DArrayView) {
      vkDestroyImageView(m_pRhi->logicDevice()->getNative(), DefaultTexture2DArrayView, nullptr);
      DefaultTexture2DArrayView = VK_NULL_HANDLE;
    }
  }
}


void Renderer::generatePbrCmds(CommandBuffer* cmdBuffer, u32 frameIndex) 
{
  GraphicsPipeline* pPipeline = nullptr;
  ComputePipeline* pCompPipeline = nullptr;
  pPipeline = RendererPass::getPipeline( GRAPHICS_PIPELINE_PBR_DEFERRED_NOLR );
  pCompPipeline = pbr_computePipeline_NoLR;

  if (m_currentGraphicsConfigs._EnableLocalReflections) {
    pPipeline = RendererPass::getPipeline( GRAPHICS_PIPELINE_PBR_DEFERRED_LR );
    pCompPipeline = pbr_computePipeline_LR;
  }

  FrameBuffer* pbr_FrameBuffer = pbr_FrameBufferKey;

  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };
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
  pbr_RenderPassInfo.framebuffer = pbr_FrameBuffer->getHandle();
  pbr_RenderPassInfo.renderPass = pbr_renderPass->getHandle();
  pbr_RenderPassInfo.pClearValues = clearValuesPBR.data();
  pbr_RenderPassInfo.clearValueCount = static_cast<u32>(clearValuesPBR.size());
  pbr_RenderPassInfo.renderArea.extent = windowExtent;
  pbr_RenderPassInfo.renderArea.offset = { 0, 0 };

  ShadowMapSystem& shadow = m_pLights->getPrimaryShadowMapSystem();

  const u32 dSetCount = 5;
#if !COMPUTE_PBR
    cmdBuffer->beginRenderPass(pbr_RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    GraphicsPipeline* pbr_Pipeline = pPipeline;
    VkDescriptorSet sets[dSetCount] = {
      m_pGlobal->getDescriptorSet(frameIndex)->getHandle(),
      pbr_DescSetKey->getHandle(),
      m_pLights->getDescriptorSet(frameIndex)->getHandle(),
      shadow.getShadowMapViewDescriptor(frameIndex)->getHandle(),
      //shadow.StaticShadowMapViewDescriptor(frameIndex)->getHandle(),
      m_pGlobalIllumination->getDescriptorSet()->getHandle(),
    };
    cmdBuffer->setViewPorts(0, 1, &viewport);
    cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_Pipeline->Pipeline());
    cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_Pipeline->getLayout(), 0, dSetCount, sets, 0, nullptr);
    VkBuffer vertexBuffer = m_RenderQuad.getQuad()->getHandle()->getNativeBuffer();
    VkBuffer indexBuffer = m_RenderQuad.getIndices()->getHandle()->getNativeBuffer();
    VkDeviceSize offsets[] = { 0 };
    cmdBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmdBuffer->bindIndexBuffer(indexBuffer, 0, getNativeIndexType(m_RenderQuad.getIndices()->GetSizeType()));
    cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();
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
    VkDescriptorSet compSets[] = { 
      m_pGlobal->getDescriptorSet(frameIndex)->getHandle(),
      pbr_DescSetKey->getHandle(),
      m_pLights->getDescriptorSet(frameIndex)->getHandle(),
      shadow.getShadowMapViewDescriptor(frameIndex)->getHandle(),
      //shadow.StaticShadowMapViewDescriptor(frameIndex)->getHandle(),
      m_pGlobalIllumination->getDescriptorSet()->getHandle(),
      pbr_compSet->getHandle()
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
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, pCompPipeline->getLayout(), 
      0, 6, compSets, 0, nullptr);
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


void Renderer::setUpOffscreen()
{
  cleanUpOffscreen();

  m_Offscreen._cmdBuffers.resize(m_pRhi->bufferingCount());
  m_Offscreen._shadowCmdBuffers.resize(m_pRhi->bufferingCount());

  m_Offscreen._semaphores.resize(m_pRhi->bufferingCount());
  m_Offscreen._shadowSemaphores.resize(m_pRhi->bufferingCount());

  VkSemaphoreCreateInfo semaCI = { };
  semaCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


  for (size_t i = 0; i < m_Offscreen._cmdBuffers.size(); ++i) {
    m_Offscreen._cmdBuffers[i] = m_pRhi->createCommandBuffer();
    m_Offscreen._cmdBuffers[i]->allocate(m_pRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    m_Offscreen._shadowCmdBuffers[i] = m_pRhi->createCommandBuffer();
    m_Offscreen._shadowCmdBuffers[i]->allocate(m_pRhi->graphicsCmdPool(1), VK_COMMAND_BUFFER_LEVEL_PRIMARY); 
    m_Offscreen._semaphores[i] = m_pRhi->createVkSemaphore();
    m_Offscreen._shadowSemaphores[i] = m_pRhi->createVkSemaphore();
    m_Offscreen._semaphores[i]->initialize(semaCI);
    m_Offscreen._shadowSemaphores[i]->initialize(semaCI);
  }
}


void Renderer::cleanUpOffscreen()
{
  for (size_t i = 0; i < m_Offscreen._cmdBuffers.size(); ++i) {
    m_pRhi->freeCommandBuffer(m_Offscreen._cmdBuffers[i]);
    m_pRhi->freeCommandBuffer(m_Offscreen._shadowCmdBuffers[i]);
    m_pRhi->freeVkSemaphore(m_Offscreen._semaphores[i]);
    m_pRhi->freeVkSemaphore(m_Offscreen._shadowSemaphores[i]);
  }
}


void Renderer::setUpDownscale(b32 FullSetUp)
{
  DescriptorSetLayout* getLayout = DownscaleBlurLayoutKey;
  DescriptorSetLayout* GlowLayout = GlowDescriptorSetLayoutKey;
  DescriptorSet* DBDS2x = m_pRhi->createDescriptorSet();
  DescriptorSet* DBDS2xFinal = m_pRhi->createDescriptorSet();
  DescriptorSet* DBDS4x = m_pRhi->createDescriptorSet();
  DescriptorSet* DBDS4xFinal = m_pRhi->createDescriptorSet();
  DescriptorSet* DBDS8x = m_pRhi->createDescriptorSet();
  DescriptorSet* DBDS8xFinal = m_pRhi->createDescriptorSet();
  DescriptorSet* DBDS16x = m_pRhi->createDescriptorSet();
  DescriptorSet* DBDS16xFinal = m_pRhi->createDescriptorSet();
  DescriptorSet* GlowDS = m_pRhi->createDescriptorSet();
  DownscaleBlurDescriptorSet2x = DBDS2x;
  DownscaleBlurDescriptorSet4x = DBDS4x;
  DownscaleBlurDescriptorSet8x = DBDS8x;
  DownscaleBlurDescriptorSet16x = DBDS16x;
  DownscaleBlurDescriptorSet2xFinalKey = DBDS2xFinal;
  DownscaleBlurDescriptorSet4xFinalKey = DBDS4xFinal;
  DownscaleBlurDescriptorSet8xFinalKey = DBDS8xFinal;
  DownscaleBlurDescriptorSet16xFinalKey = DBDS16xFinal;
  GlowDescriptorSetKey = GlowDS;

  DBDS2x->allocate(m_pRhi->descriptorPool(), getLayout);
  DBDS4x->allocate(m_pRhi->descriptorPool(), getLayout);
  DBDS8x->allocate(m_pRhi->descriptorPool(), getLayout);
  DBDS16x->allocate(m_pRhi->descriptorPool(), getLayout);
  DBDS2xFinal->allocate(m_pRhi->descriptorPool(), getLayout);
  DBDS4xFinal->allocate(m_pRhi->descriptorPool(), getLayout);
  DBDS8xFinal->allocate(m_pRhi->descriptorPool(), getLayout);
  DBDS16xFinal->allocate(m_pRhi->descriptorPool(), getLayout);
  GlowDS->allocate(m_pRhi->descriptorPool(), GlowLayout);

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
  Img.sampler = gbuffer_Sampler->getHandle();
  Img.imageView = RTBright->getView();
  Img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  Log(rNotify) << Img.imageLayout << "\n";
  
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
  Log(rNotify) << WriteSet.descriptorType << "\n";
  
  DBDS2x->update(1, &WriteSet);
  Img.imageView = Color2x->getView();
  Img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  DBDS2xFinal->update(1, &WriteSet);
  Img.imageView = Color2xFinal->getView();
  DBDS4x->update(1, &WriteSet);
  Img.imageView = Color4x->getView();
  DBDS4xFinal->update(1, &WriteSet);
  Img.imageView = Color4xFinal->getView();
  DBDS8x->update(1, &WriteSet);
  Img.imageView = Color8x->getView();
  DBDS8xFinal->update(1, &WriteSet);
  Img.imageView = Color8xFinal->getView();
  DBDS16x->update(1, &WriteSet);
  Img.imageView = Color16x->getView();
  DBDS16xFinal->update(1, &WriteSet);

  Img.imageView = Color2xFinal->getView();

  VkDescriptorImageInfo Img1 = { };
  Img1.sampler = gbuffer_Sampler->getHandle();
  Img1.imageView = Color4xFinal->getView();
  Img1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkDescriptorImageInfo Img2 = { };
  Img2.sampler = gbuffer_Sampler->getHandle();
  Img2.imageView = Color8xFinal->getView();
  Img2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkDescriptorImageInfo Img3 = { };
  Img3.sampler = gbuffer_Sampler->getHandle();
  Img3.imageView = Color16xFinal->getView();
  Img3.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

  GlowDS->update(static_cast<u32>(GlowWrites.size()), GlowWrites.data());
}


void Renderer::cleanUpDownscale(b32 FullCleanUp)
{

  DescriptorSet* DBDS2x = DownscaleBlurDescriptorSet2x;
  m_pRhi->freeDescriptorSet(DBDS2x);
  DescriptorSet* DBDS4x = DownscaleBlurDescriptorSet4x;
  m_pRhi->freeDescriptorSet(DBDS4x);
  DescriptorSet* DBDS8x = DownscaleBlurDescriptorSet8x;
  m_pRhi->freeDescriptorSet(DBDS8x);
  DescriptorSet* DBDS16x = DownscaleBlurDescriptorSet16x;
  m_pRhi->freeDescriptorSet(DBDS16x);
  DescriptorSet* DBDS2xFinal = DownscaleBlurDescriptorSet2xFinalKey;
  m_pRhi->freeDescriptorSet(DBDS2xFinal);
  DescriptorSet* DBDS4xFinal = DownscaleBlurDescriptorSet4xFinalKey;
  m_pRhi->freeDescriptorSet(DBDS4xFinal);
  DescriptorSet* DBDS8xFinal = DownscaleBlurDescriptorSet8xFinalKey;
  m_pRhi->freeDescriptorSet(DBDS8xFinal);
  DescriptorSet* DBDS16xFinal = DownscaleBlurDescriptorSet16xFinalKey;
  m_pRhi->freeDescriptorSet(DBDS16xFinal);
  DescriptorSet* GlowDS = GlowDescriptorSetKey;
  m_pRhi->freeDescriptorSet(GlowDS);
}


void Renderer::setUpHDR(b32 fullSetUp)
{
  cleanUpHDR(fullSetUp);
  if (fullSetUp) {
    m_HDR._CmdBuffers.resize(m_pRhi->bufferingCount());
    for (u32 i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
      m_HDR._CmdBuffers[i] = m_pRhi->createCommandBuffer();
      m_HDR._CmdBuffers[i]->allocate(m_pRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
  }

  VkSemaphoreCreateInfo semaCi = {};
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_HDR._semaphores.resize(m_pRhi->bufferingCount());
  for (u32 i = 0; i < m_HDR._semaphores.size(); ++i) {
    m_HDR._semaphores[i] = m_pRhi->createVkSemaphore();
    m_HDR._semaphores[i]->initialize(semaCi);
  }

  switch (m_currentGraphicsConfigs._AA) {
  case AA_FXAA_2x: m_pAntiAliasingFXAA->updateSets(m_pRhi, m_pGlobal);
  }

  DescriptorSet* hdrSet = m_pRhi->createDescriptorSet();
  hdr_gamma_descSetKey = hdrSet;
  std::array<VkWriteDescriptorSet, 2> hdrWrites;

  VkDescriptorImageInfo pbrImageInfo = { };
  switch (m_currentGraphicsConfigs._AA) {
    case AA_FXAA_2x:
    {
      Texture* texture = m_pAntiAliasingFXAA->GetOutput();
      Sampler* sampler = m_pAntiAliasingFXAA->GetOutputSampler();
      pbrImageInfo.sampler = sampler->getHandle();
      pbrImageInfo.imageView = texture->getView();
      pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } break;
    case AA_None:
    default:  
    {
      pbrImageInfo.sampler = gbuffer_SamplerKey->getHandle();
      pbrImageInfo.imageView = pbr_FinalTextureKey->getView();
      pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
  }
  // TODO(): We don't have our bloom pipeline and texture yet, we will sub it with this instead!
  VkDescriptorImageInfo bloomImageInfo = { };
  bloomImageInfo.sampler = gbuffer_SamplerKey->getHandle();
  bloomImageInfo.imageView = RenderTargetGlowKey->getView();
  bloomImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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
  hdrSet->allocate(m_pRhi->descriptorPool(), hdr_gamma_descSetLayoutKey);
  hdrSet->update(static_cast<u32>(hdrWrites.size()), hdrWrites.data());
}


void Renderer::cleanUpHDR(b32 fullCleanUp)
{
  if (fullCleanUp) {
    for (u32 i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
      m_pRhi->freeCommandBuffer(m_HDR._CmdBuffers[i]);
      m_HDR._CmdBuffers[i] = nullptr;
    }
  }

  if (hdr_gamma_descSetKey) {
    m_pRhi->freeDescriptorSet(hdr_gamma_descSetKey);
    hdr_gamma_descSetKey = nullptr;
  }

  for (u32 i = 0; i < m_HDR._semaphores.size(); ++i) {
    m_pRhi->freeVkSemaphore(m_HDR._semaphores[i]);
    m_HDR._semaphores[i] = nullptr;
  }
}


void Renderer::buildOffScreenCmdList()
{
  R_ASSERT(m_pRhi->bufferingCount() == m_Offscreen._cmdBuffers.size(), "Attempted to build offscreen cmd buffer. Index out of bounds!\n");
  for (u32 i = 0; i < m_Offscreen._cmdBuffers.size(); ++i) {
    CommandBuffer* cmdBuf = m_Offscreen._cmdBuffers[i];
    R_ASSERT(cmdBuf, "Offscreen cmd buffer is null.");
    cmdBuf->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdBuf->begin(beginInfo);
    generateOffScreenCmds(cmdBuf, i);
    cmdBuf->end();
  }
}


void Renderer::buildHDRCmdList()
{
  for (u32 i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_HDR._CmdBuffers[i];
    R_ASSERT(pCmdBuffer, "HDR buffer is null");
    pCmdBuffer->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo cmdBi = {};
    cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    pCmdBuffer->begin(cmdBi);
    generateHDRCmds(pCmdBuffer, i);
    pCmdBuffer->end();
  }
}


void Renderer::buildShadowCmdList()
{
  u32 frameIndex = m_pRhi->currentFrame();
  CommandBuffer* shadowBuf = m_Offscreen._shadowCmdBuffers[frameIndex];
  R_ASSERT(shadowBuf, "Shadow Buffer is null.");
  shadowBuf->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  VkCommandBufferBeginInfo begin = {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  shadowBuf->begin(begin);
  generateShadowCmds(shadowBuf, frameIndex);
  shadowBuf->end();
}


void Renderer::buildPbrCmdLists()
{
  for (u32 i = 0; i < m_Pbr._CmdBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_Pbr._CmdBuffers[i];
    R_ASSERT(pCmdBuffer, "PBR command buffer is null.");
    pCmdBuffer->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    pCmdBuffer->begin(beginInfo);
    generatePbrCmds(pCmdBuffer, i);
    pCmdBuffer->end();
  }
}


void Renderer::buildSkyboxCmdLists()
{
  for (u32 i = 0; i < m_pSkyboxCmdBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_pSkyboxCmdBuffers[i];
    R_ASSERT(pCmdBuffer, "Skybox buffer is null");
    pCmdBuffer->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    pCmdBuffer->begin(beginInfo);
    generateSkyboxCmds(pCmdBuffer, i);
    pCmdBuffer->end();
  }
}


void Renderer::buildFinalCmdLists()
{
  for (u32 i = 0; i < m_pFinalCommandBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_pFinalCommandBuffers[i];
    R_ASSERT(pCmdBuffer, "Final Command buffer is null.");
    pCmdBuffer->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo cmdBi = {};
    cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    pCmdBuffer->begin(cmdBi);
    generateFinalCmds(pCmdBuffer);
    pCmdBuffer->end();
  }
}


void Renderer::build()
{
  m_pRhi->waitAllGraphicsQueues();

  updateSceneDescriptors(m_pRhi->currentFrame());

  buildOffScreenCmdList();
  buildHDRCmdList();
  buildShadowCmdList();
  buildPbrCmdLists();
  buildSkyboxCmdLists();
  buildFinalCmdLists();
  m_pRhi->rebuildCommandBuffers(m_pRhi->currentSwapchainCmdBufferSet());
}


void Renderer::updateSkyboxCubeMap()
{
  DescriptorSet* skyboxSet = skybox_descriptorSetKey;
  Texture* cubemap = m_pSky->GetCubeMap();
  if (m_usePreRenderSkybox) {
    cubemap = m_preRenderSkybox->getHandle(); 
  }
  VkDescriptorImageInfo image = {};
  image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image.imageView = cubemap->getView();
  image.sampler = m_pSky->GetSampler()->getHandle();

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstArrayElement = 0;
  write.dstBinding = 0;
  write.pImageInfo = &image;

  skyboxSet->update(1, &write);
}


void Renderer::setUpSkybox(b32 justSemaphores)
{
  cleanUpSkybox(justSemaphores);
  if (!justSemaphores) {
    DescriptorSet* skyboxSet = m_pRhi->createDescriptorSet();
    skybox_descriptorSetKey = skyboxSet;
    DescriptorSetLayout* layout = skybox_setLayoutKey;

    skyboxSet->allocate(m_pRhi->descriptorPool(), layout);

    updateSkyboxCubeMap();
  }

  // Create skybox Commandbuffer.
  m_pSkyboxCmdBuffers.resize(m_pRhi->bufferingCount());
  for (u32 i = 0; i < m_pSkyboxCmdBuffers.size(); ++i) {
    m_pSkyboxCmdBuffers[i] = m_pRhi->createCommandBuffer();
    m_pSkyboxCmdBuffers[i]->allocate(m_pRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  }
  m_SkyboxFinishedSignals.resize(m_pRhi->bufferingCount());
  VkSemaphoreCreateInfo sema = {};
  sema.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (u32 i = 0; i < m_SkyboxFinishedSignals.size(); ++i) {
    m_SkyboxFinishedSignals[i] = m_pRhi->createVkSemaphore();
    m_SkyboxFinishedSignals[i]->initialize(sema);
  }
}


void Renderer::usePreRenderSkyboxMap(b32 enable)
{
  m_usePreRenderSkybox = enable;
  m_pRhi->graphicsWaitIdle(0);
  updateSkyboxCubeMap();
  updateGlobalIlluminationBuffer();

  buildSkyboxCmdLists();
  buildPbrCmdLists();
}


void Renderer::cleanUpSkybox(b32 justSemaphores)
{
  if (!justSemaphores) {
    DescriptorSet* skyboxSet = skybox_descriptorSetKey;
    m_pRhi->freeDescriptorSet(skyboxSet);
  }
  for (u32 i = 0; i < m_SkyboxFinishedSignals.size(); ++i) {
    m_pRhi->freeVkSemaphore(m_SkyboxFinishedSignals[i]);
    m_SkyboxFinishedSignals[i] = nullptr;
  }

  for (u32 i = 0; i < m_pSkyboxCmdBuffers.size(); ++i) {
    // Cleanup commandbuffer for skybox.
    m_pRhi->freeCommandBuffer(m_pSkyboxCmdBuffers[i]);
    m_pSkyboxCmdBuffers[i] = nullptr;
  }
}


void Renderer::generateSkyboxCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{
  R_TIMED_PROFILE_RENDERER();
  
  CommandBuffer* buf = cmdBuffer;
  FrameBuffer* skyFrameBuffer = pbr_FrameBufferKey;
  GraphicsPipeline* skyPipeline = skybox_pipelineKey;
  DescriptorSet* global = m_pGlobal->getDescriptorSet(frameIndex);
  DescriptorSet* skybox = skybox_descriptorSetKey;

  VkDescriptorSet descriptorSets[] = {
    global->getHandle(),
    skybox->getHandle()
  };  

  std::array<VkClearValue, 3> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].depthStencil = { 1.0f, 0 };

  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };
  VkViewport viewport = {};
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  VkRenderPassBeginInfo renderBegin = { };
  renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderBegin.framebuffer = skyFrameBuffer->getHandle();
  renderBegin.renderPass = m_pSky->GetSkyboxRenderPass()->getHandle();
  renderBegin.clearValueCount = static_cast<u32>(clearValues.size());
  renderBegin.pClearValues = clearValues.data();
  renderBegin.renderArea.offset = { 0, 0 };
  renderBegin.renderArea.extent = windowExtent;
    
  // Start the renderpass.
  buf->beginRenderPass(renderBegin, VK_SUBPASS_CONTENTS_INLINE);
    buf->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->Pipeline());
    buf->setViewPorts(0, 1, &viewport);
    buf->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->getLayout(), 0, 2, descriptorSets, 0, nullptr);
    VertexBuffer* vertexbuffer = m_pSky->GetSkyboxVertexBuffer();
    IndexBuffer* idxBuffer = m_pSky->GetSkyboxIndexBuffer();

    VkDeviceSize offsets[] =  { 0 };
    VkBuffer vert = vertexbuffer->getHandle()->getNativeBuffer();
    VkBuffer ind = idxBuffer->getHandle()->getNativeBuffer();
    buf->bindVertexBuffers(0 , 1, &vert, offsets);  
    buf->bindIndexBuffer(ind, 0, getNativeIndexType(idxBuffer->GetSizeType()));
    buf->drawIndexed(idxBuffer->IndexCount(), 1, 0, 0, 0);
  buf->endRenderPass();
}


void Renderer::generateOffScreenCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{
  R_TIMED_PROFILE_RENDERER();

  if (!m_pLights || !m_pGlobal) {  
    Log(rWarning) << "Can not build commandbuffers without light or global data! One of them is null!";
  } 

  FrameBuffer* gbuffer_FrameBuffer = gbuffer_FrameBufferKey;
  GraphicsPipeline* gbuffer_Pipeline = RendererPass::getPipeline( GRAPHICS_PIPELINE_GBUFFER_DYNAMIC );
  GraphicsPipeline* gbuffer_StaticPipeline = RendererPass::getPipeline( GRAPHICS_PIPELINE_GBUFFER_STATIC );
  GraphicsPipeline* gbuffer_staticMorph = RendererPass::getPipeline( GRAPHICS_PIPELINE_GBUFFER_STATIC_MORPH_TARGETS );
  GraphicsPipeline* gbuffer_dynamicMorph = RendererPass::getPipeline( GRAPHICS_PIPELINE_GBUFFER_DYNAMIC_MORPH_TARGETS );
  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };
  VkDescriptorSet DescriptorSets[6];

  {
    std::array<VkClearValue, 5> clearValues;
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[4].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo gbuffer_RenderPassInfo = {};
    gbuffer_RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    gbuffer_RenderPassInfo.framebuffer = gbuffer_FrameBuffer->getHandle();
    gbuffer_RenderPassInfo.renderPass = gbuffer_renderPass->getHandle();
    gbuffer_RenderPassInfo.pClearValues = clearValues.data();
    gbuffer_RenderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
    gbuffer_RenderPassInfo.renderArea.extent = windowExtent;
    gbuffer_RenderPassInfo.renderArea.offset = { 0, 0 };

    cmdBuffer->beginRenderPass(gbuffer_RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  }

  VkViewport viewport =  { };
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  if (m_cmdDeferredList.Size() == 0) {
    VkClearAttachment clearAttachments[5];
    for (u32 i = 0; i < 4; ++i) {
      clearAttachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      clearAttachments[i].clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
      clearAttachments[i].colorAttachment = i;
    }
    clearAttachments[4].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    clearAttachments[4].clearValue.depthStencil = { 1.0f, 0 };
    clearAttachments[4].colorAttachment = 4;
    VkClearRect clearRects[4] = { };
    for (u32 i = 0; i < 4; ++i) {
      clearRects[i].baseArrayLayer = 0;
      clearRects[i].layerCount = 0;
      clearRects[i].rect.extent = { m_renderWidth, m_renderHeight };
      clearRects[i].rect.offset = { 0, 0 };
    }
    cmdBuffer->clearAttachments(5, clearAttachments, 4, clearRects);
  } else {
    for (size_t i = 0; i < m_cmdDeferredList.Size(); ++i) {
      PrimitiveRenderCmd& renderCmd = m_cmdDeferredList.get(i);
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
      VertexBuffer* vertexBuffer = data->getVertexData();
      IndexBuffer* indexBuffer = data->getIndexData();
      VkBuffer vb = vertexBuffer->getHandle()->getNativeBuffer();
      VkDeviceSize offsets[] = { 0 };
      cmdBuffer->bindVertexBuffers(0, 1, &vb, offsets);
      if (renderCmd._config & CMD_MORPH_BIT) {
        Pipe = Skinned ? gbuffer_dynamicMorph : gbuffer_staticMorph;
        R_ASSERT(renderCmd._pMorph0, "morph0 is null");
        R_ASSERT(renderCmd._pMorph1, "morph1 is null.");
        VkBuffer morph0 = renderCmd._pMorph0->getVertexData()->getHandle()->getNativeBuffer();
        VkBuffer morph1 = renderCmd._pMorph1->getVertexData()->getHandle()->getNativeBuffer();
        cmdBuffer->bindVertexBuffers(1, 1, &morph0, offsets);
        cmdBuffer->bindVertexBuffers(2, 1,  &morph1, offsets);
      } 

      cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipe->Pipeline());
      cmdBuffer->setViewPorts(0, 1, &viewport);

      DescriptorSets[0] = m_pGlobal->getDescriptorSet(frameIndex)->getHandle();
      DescriptorSets[1] = pMeshDesc->getCurrMeshSet(frameIndex)->getHandle();
      DescriptorSets[3] = (Skinned ? renderCmd._pJointDesc->getCurrJointSet(frameIndex)->getHandle() : nullptr);

      if (indexBuffer) {
        VkBuffer ib = indexBuffer->getHandle()->getNativeBuffer();
        cmdBuffer->bindIndexBuffer(ib, 0, getNativeIndexType(indexBuffer->GetSizeType()));
      }

      MaterialDescriptor* pMatDesc = renderCmd._pPrimitive->_pMat->getNative();
      DescriptorSets[2] = pMatDesc->CurrMaterialSet()->getHandle();
      // Bind materials.
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, 
        Pipe->getLayout(), 0, (Skinned ? 4 : 3), DescriptorSets, 0, nullptr);
      if (indexBuffer) {
        cmdBuffer->drawIndexed(renderCmd._pPrimitive->_indexCount, renderCmd._instances, 
          renderCmd._pPrimitive->_firstIndex, 0, 0);
      } else {
        cmdBuffer->draw(vertexBuffer->VertexCount(), renderCmd._instances, 0, 0);
      }
    }
  }

  cmdBuffer->endRenderPass();

  // Build decals after.
  m_decalEngine->buildDecals(cmdBuffer);
}


void Renderer::generateFinalCmds(CommandBuffer* cmdBuffer)
{
  R_TIMED_PROFILE_RENDERER();

  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };
  // Do stuff with the buffer.
  VkViewport viewport = {};
  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.x = 0.0f;
  viewport.y = 0.0f;

  VkRect2D scissor = { };
  scissor.extent = { m_renderWidth, m_renderHeight };
  scissor.offset = { 0, 0 };

  GraphicsPipeline* finalPipeline = final_PipelineKey;
  DescriptorSet* finalSet = final_DescSetKey;
  FrameBuffer* finalFrameBuffer = final_frameBufferKey;

  VkClearValue clearVal = {};
  clearVal.color = { 0.0f, 0.0f, 0.0f, 1.0f };

  VkRenderPassBeginInfo renderpassInfo = { };
  renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderpassInfo.framebuffer = finalFrameBuffer->getHandle();
  renderpassInfo.clearValueCount = 1;
  renderpassInfo.pClearValues = &clearVal;
  renderpassInfo.renderPass = final_renderPass->getHandle();
  renderpassInfo.renderArea.extent = windowExtent;
  renderpassInfo.renderArea.offset = { 0, 0 };

  cmdBuffer->beginRenderPass(renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->setViewPorts(0, 1, &viewport);
    cmdBuffer->setScissor(0, 1, &scissor);
    cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Pipeline());
    VkDescriptorSet finalDescriptorSets[] = { finalSet->getHandle() };

    cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->getLayout(), 0, 1, finalDescriptorSets, 0, nullptr);
    VkBuffer vertexBuffer = m_RenderQuad.getQuad()->getHandle()->getNativeBuffer();
    VkBuffer indexBuffer = m_RenderQuad.getIndices()->getHandle()->getNativeBuffer();
    VkDeviceSize offsets[] = { 0 };

    cmdBuffer->bindIndexBuffer(indexBuffer, 0, getNativeIndexType(m_RenderQuad.getIndices()->GetSizeType()));
    cmdBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);

    cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
  cmdBuffer->endRenderPass();
}


void Renderer::generateHDRCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{


  VkIndexType indexType = getNativeIndexType(m_RenderQuad.getIndices()->GetSizeType());
  VkBuffer vertexBuffer = m_RenderQuad.getQuad()->getHandle()->getNativeBuffer();
  VkBuffer indexBuffer = m_RenderQuad.getIndices()->getHandle()->getNativeBuffer();
  VkDeviceSize offsets[] = { 0 };

  GraphicsPipeline* hdrPipeline = RendererPass::getPipeline( GRAPHICS_PIPELINE_HDR_GAMMA );
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
  renderpassInfo.framebuffer = hdrFrameBuffer->getHandle();
  renderpassInfo.clearValueCount = 1;
  renderpassInfo.pClearValues = &clearVal;
  renderpassInfo.renderPass = hdr_renderPass->getHandle();
  renderpassInfo.renderArea.extent = { m_renderWidth, m_renderHeight };
  renderpassInfo.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass2x =  { };
  DownscalePass2x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass2x.framebuffer = DownscaleFrameBuffer2x->getHandle();
  DownscalePass2x.renderPass = DownscaleFrameBuffer2x->RenderPassRef()->getHandle();
  DownscalePass2x.clearValueCount = 1;
  DownscalePass2x.pClearValues = &clearVal;
  DownscalePass2x.renderArea.extent = { DownscaleFrameBuffer2x->getWidth(), DownscaleFrameBuffer2x->getHeight() };
  DownscalePass2x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass4x = { };
  DownscalePass4x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass4x.framebuffer = DownscaleFrameBuffer4x->getHandle();
  DownscalePass4x.renderPass = DownscaleFrameBuffer4x->RenderPassRef()->getHandle();
  DownscalePass4x.clearValueCount = 1;
  DownscalePass4x.pClearValues = &clearVal;
  DownscalePass4x.renderArea.extent = { DownscaleFrameBuffer4x->getWidth(), DownscaleFrameBuffer4x->getHeight() };
  DownscalePass4x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass8x = {};
  DownscalePass8x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass8x.framebuffer = DownscaleFrameBuffer8x->getHandle();
  DownscalePass8x.renderPass = DownscaleFrameBuffer8x->RenderPassRef()->getHandle();
  DownscalePass8x.clearValueCount = 1;
  DownscalePass8x.pClearValues = &clearVal;
  DownscalePass8x.renderArea.extent = { DownscaleFrameBuffer8x->getWidth(), DownscaleFrameBuffer8x->getHeight() };
  DownscalePass8x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo DownscalePass16x = {};
  DownscalePass16x.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  DownscalePass16x.framebuffer = DownscaleFrameBuffer16x->getHandle();
  DownscalePass16x.renderPass = DownscaleFrameBuffer16x->RenderPassRef()->getHandle();
  DownscalePass16x.clearValueCount = 1;
  DownscalePass16x.pClearValues = &clearVal;
  DownscalePass16x.renderArea.extent = { DownscaleFrameBuffer16x->getWidth(), DownscaleFrameBuffer16x->getHeight() };
  DownscalePass16x.renderArea.offset = { 0, 0 };

  VkRenderPassBeginInfo GlowPass = {};
  GlowPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  GlowPass.framebuffer = GlowFrameBuffer->getHandle();
  GlowPass.renderPass = GlowFrameBuffer->RenderPassRef()->getHandle();
  GlowPass.clearValueCount = 1;
  GlowPass.pClearValues = &clearVal;
  GlowPass.renderArea.extent = { GlowFrameBuffer->getWidth(), GlowFrameBuffer->getHeight() };
  GlowPass.renderArea.offset = { 0, 0 };

  VkViewport viewport = {};
  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };
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
    VkDescriptorSet DownscaleSetNative = DownscaleSet2x->getHandle();
    viewport.height = (r32)(windowExtent.height >> 1);
    viewport.width =  (r32)(windowExtent.width  >> 1);
    cmdBuffer->beginRenderPass(DownscalePass2x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->setViewPorts(0, 1, &viewport);
      cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Pipeline());
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->getLayout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->bindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->pushConstants(Downscale2x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(r32), &m_Downscale._Horizontal);
      cmdBuffer->pushConstants(Downscale2x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 4, sizeof(r32), &m_Downscale._Strength);
      cmdBuffer->pushConstants(Downscale2x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 8, sizeof(r32), &m_Downscale._Scale);
      cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();
    DownscalePass2x.framebuffer = FB2xFinal->getHandle();
    DownscalePass2x.renderPass = FB2xFinal->RenderPassRef()->getHandle();
    cmdBuffer->beginRenderPass(DownscalePass2x, VK_SUBPASS_CONTENTS_INLINE);
      m_Downscale._Horizontal = false;
      DownscaleSetNative = DownscaleSet2xFinal->getHandle();
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->getLayout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->pushConstants(Downscale2x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &m_Downscale._Horizontal);
      cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();

    viewport.height = (r32)(windowExtent.height >> 2);
    viewport.width = (r32)(windowExtent.width   >> 2);
    DownscaleSetNative = DownscaleSet4x->getHandle();
    i32 _Horizontal = true;
    cmdBuffer->beginRenderPass(DownscalePass4x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->setViewPorts(0, 1, &viewport);
      cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Pipeline());
      cmdBuffer->pushConstants(Downscale4x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->getLayout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->bindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();

    DownscalePass4x.framebuffer = FB4xFinal->getHandle();
    DownscalePass4x.renderPass = FB4xFinal->RenderPassRef()->getHandle();
    cmdBuffer->beginRenderPass(DownscalePass4x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet4xFinal->getHandle();
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->getLayout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->pushConstants(Downscale4x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();

    viewport.height = (r32)(windowExtent.height >> 3);
    viewport.width = (r32)(windowExtent.width   >> 3);
    DownscaleSetNative = DownscaleSet8x->getHandle();
    _Horizontal = true;
    cmdBuffer->beginRenderPass(DownscalePass8x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->setViewPorts(0, 1, &viewport);
      cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Pipeline());
      cmdBuffer->pushConstants(Downscale8x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->getLayout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->bindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();

    DownscalePass8x.framebuffer = FB8xFinal->getHandle();
    DownscalePass8x.renderPass = FB8xFinal->RenderPassRef()->getHandle();
    cmdBuffer->beginRenderPass(DownscalePass8x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet8xFinal->getHandle();
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->getLayout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->pushConstants(Downscale4x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();

    viewport.height = (r32)(windowExtent.height >> 4);
    viewport.width = (r32)(windowExtent.width   >> 4);
    DownscaleSetNative = DownscaleSet16x->getHandle();
    _Horizontal = true;
    cmdBuffer->beginRenderPass(DownscalePass16x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->setViewPorts(0, 1, &viewport);
      cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->Pipeline());
      cmdBuffer->pushConstants(Downscale16x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->getLayout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->bindIndexBuffer(indexBuffer, 0, indexType);
      cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();

    DownscalePass16x.framebuffer = FB16xFinal->getHandle();
    DownscalePass16x.renderPass = FB16xFinal->RenderPassRef()->getHandle();
    cmdBuffer->beginRenderPass(DownscalePass16x, VK_SUBPASS_CONTENTS_INLINE);
      _Horizontal = false;
      DownscaleSetNative = DownscaleSet16xFinal->getHandle();
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale16x->getLayout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->pushConstants(Downscale16x->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &_Horizontal);
      cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->endRenderPass();
  }

  viewport.height = (r32)windowExtent.height;
  viewport.width = (r32)windowExtent.width;
  VkDescriptorSet GlowDescriptorNative = GlowSet->getHandle();
  cmdBuffer->beginRenderPass(GlowPass, VK_SUBPASS_CONTENTS_INLINE);
  if (!m_currentGraphicsConfigs._EnableBloom) {
    VkClearAttachment clearAttachment = {};
    clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clearAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearAttachment.colorAttachment = 0;
    VkClearRect rect = {};
    rect.baseArrayLayer = 0;
    rect.layerCount = 1;
    VkExtent2D extent = { m_renderWidth, m_renderHeight };
    rect.rect.extent = extent;
    rect.rect = { 0, 0 };
    cmdBuffer->clearAttachments(1, &clearAttachment, 1, &rect);
  } else {
    cmdBuffer->setViewPorts(0, 1, &viewport);
    cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Pipeline());
    cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->getLayout(), 0, 1, &GlowDescriptorNative, 0, nullptr);
    cmdBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmdBuffer->bindIndexBuffer(indexBuffer, 0, indexType);
    cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
  }
  cmdBuffer->endRenderPass();

  

  VkDescriptorSet dSets[3];
  dSets[0] = m_pGlobal->getDescriptorSet(frameIndex)->getHandle();
  dSets[1] = hdrSet->getHandle();
  dSets[2] = m_pHDR->getSet()->getHandle();
  
  if (m_currentGraphicsConfigs._AA == AA_FXAA_2x) {
    m_pAntiAliasingFXAA->generateCommands(m_pRhi, cmdBuffer, m_pGlobal, frameIndex);
  }

  cmdBuffer->beginRenderPass(renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->setViewPorts(0, 1, &viewport);
    cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Pipeline());
    cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->getLayout(), 0, 3, dSets, 0, nullptr);
    cmdBuffer->pushConstants(hdrPipeline->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ParamsHDR), &m_HDR._pushCnst);
    cmdBuffer->bindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmdBuffer->bindIndexBuffer(indexBuffer, 0, indexType);
    cmdBuffer->drawIndexed(m_RenderQuad.getIndices()->IndexCount(), 1, 0, 0, 0);
  cmdBuffer->endRenderPass();
}


void Renderer::adjustHDRSettings(const ParamsHDR& hdrSettings)
{
  m_HDR._pushCnst = hdrSettings;
  for (u32 i = 0; i < m_HDR._CmdBuffers.size(); ++i) {
    CommandBuffer* pCmdBuffer = m_HDR._CmdBuffers[i];
    R_ASSERT(pCmdBuffer, "HDR buffer is null");
    pCmdBuffer->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    VkCommandBufferBeginInfo cmdBi = { };
    cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  
    pCmdBuffer->begin(cmdBi);
    generateHDRCmds(pCmdBuffer, i);
    pCmdBuffer->end();
  }
}


void Renderer::generateShadowCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
{
  R_TIMED_PROFILE_RENDERER();

  ShadowMapSystem& system = m_pLights->getPrimaryShadowMapSystem();
  system.generateDynamicShadowCmds(cmdBuffer, m_dynamicCmdList, frameIndex);
  //if (system.staticMapNeedsUpdate()) {
  //  system.GenerateStaticShadowCmds(cmdBuffer, m_staticCmdList, frameIndex);
  //  SignalStaticUpdate();
  //}
#if 0 
  R_TIMED_PROFILE_RENDERER();
  if (!m_pLights) return;
  if (!m_pLights->m_pFrameBuffer) return;

  GraphicsPipeline* staticPipeline = ShadowMapPipelineKey;
  GraphicsPipeline* dynamicPipeline = DynamicShadowMapPipelineKey;  
  DescriptorSet*    lightViewSet = m_pLights->m_pLightViewDescriptorSet;  

  VkRenderPassBeginInfo renderPass = { };
  renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPass.framebuffer = m_pLights->m_pFrameBuffer->getHandle();
  renderPass.renderPass = m_pLights->m_pFrameBuffer->RenderPassRef()->getHandle();
  renderPass.renderArea.extent = { m_pLights->m_pFrameBuffer->getWidth(), m_pLights->m_pFrameBuffer->getHeight() };
  renderPass.renderArea.offset = { 0, 0 };
  VkClearValue depthValue = { };
  depthValue.depthStencil = { 1.0f, 0 };
  renderPass.clearValueCount = 1;
  renderPass.pClearValues = &depthValue;

  VkViewport viewport = {};
  viewport.height = (r32)m_pLights->m_pFrameBuffer->getHeight();
  viewport.width = (r32)m_pLights->m_pFrameBuffer->getWidth();
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
    descriptorSets[0] = pMeshDesc->getCurrMeshSet()->getHandle();
    descriptorSets[1] = lightViewSet->getHandle();
    descriptorSets[3] = skinned ? renderCmd._pJointDesc->getCurrJointSet()->getHandle() : VK_NULL_HANDLE;

    GraphicsPipeline* pipeline = skinned ? dynamicPipeline : staticPipeline;
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Pipeline());
    cmdBuffer->SetViewPorts(0, 1, &viewport);
   MeshData* mesh = renderCmd._pMeshData;
    VertexBuffer* vertex = mesh->getVertexData();
    IndexBuffer* index = mesh->getIndexData();
    VkBuffer buf = vertex->getHandle()->NativeBuffer();
    VkDeviceSize offset[] = { 0 };
    cmdBuffer->BindVertexBuffers(0, 1, &buf, offset);
    if (index) {
      VkBuffer ind = index->getHandle()->NativeBuffer();
      cmdBuffer->BindIndexBuffer(ind, 0, getNativeIndexType(index->GetSizeType()));
    }

    Primitive* primitives = mesh->getPrimitiveData();
    u32 count = mesh->getPrimitiveCount();
    for (u32 i = 0; i < count; ++i) {
      Primitive& primitive = primitives[i];
      descriptorSets[2] = primitive._pMat->CurrMaterialSet()->getHandle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, skinned ? 4 : 3, descriptorSets, 0, nullptr);
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


void Renderer::setUpForwardPBR()
{
  m_Forward._cmdBuffers.resize(m_pRhi->bufferingCount());
  m_Forward._semaphores.resize(m_pRhi->bufferingCount());
  VkSemaphoreCreateInfo semaCi = {};
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (u32 i = 0; i < m_Forward._cmdBuffers.size(); ++i) {
    m_Forward._cmdBuffers[i] = m_pRhi->createCommandBuffer();
    CommandBuffer* cmdB = m_Forward._cmdBuffers[i];
    cmdB->allocate(m_pRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  
    m_Forward._semaphores[i] = m_pRhi->createVkSemaphore();
    m_Forward._semaphores[i]->initialize(semaCi);
  }

}


void Renderer::cleanUpForwardPBR()
{
  for (u32 i = 0; i < m_Forward._cmdBuffers.size(); ++i) {
    m_pRhi->freeCommandBuffer(m_Forward._cmdBuffers[i]);
    m_pRhi->freeVkSemaphore(m_Forward._semaphores[i]);
  }
}


void Renderer::generateForwardPBRCmds(CommandBuffer* cmdBuffer, u32 frameIndex)
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
  VkExtent2D windowExtent = { m_renderWidth, m_renderHeight };
  renderPassCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassCi.framebuffer = pbr_forwardFrameBuffer->getHandle();
  renderPassCi.renderPass = pbr_forwardRenderPass->getHandle();
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

  cmdBuffer->beginRenderPass(renderPassCi, VK_SUBPASS_CONTENTS_INLINE);
    for (size_t i = 0; i < m_forwardCmdList.Size(); ++i) {
      PrimitiveRenderCmd& renderCmd = m_forwardCmdList[i];
      if (!(renderCmd._config & CMD_FORWARD_BIT) && !(renderCmd._config & CMD_RENDERABLE_BIT)) continue;
      R_ASSERT(renderCmd._pMeshData, "Null mesh data was passed to renderer.");
      MeshDescriptor* pMeshDesc = renderCmd._pMeshDesc;
      b32 Skinned = (renderCmd._config & CMD_SKINNED_BIT);
      b32 debugging = (renderCmd._config & CMD_DEBUG_BIT);
      // Set up the render mesh
      MeshData* data = renderCmd._pMeshData;

      VertexBuffer* vertexBuffer = data->getVertexData();
      IndexBuffer* indexBuffer = data->getIndexData();
      VkBuffer vb = vertexBuffer->getHandle()->getNativeBuffer();
      VkDeviceSize offsets[] = { 0 };

      GraphicsPipeline* Pipe = (Skinned ? 
        (debugging ? skinPipelineDebug : skinPipeline) 
        : (debugging ? staticPipelineDebug : staticPipeline));
      cmdBuffer->bindVertexBuffers(0, 1, &vb, offsets);
      if (renderCmd._config & CMD_MORPH_BIT) {
        Pipe = (Skinned ? 
          (debugging ? skinMorphPipelineDebug : skinMorphPipeline) 
          : (debugging ? staticMorphPipelineDebug : staticMorphPipeline));
        R_ASSERT(renderCmd._pMorph0, "morph0 is null");
        R_ASSERT(renderCmd._pMorph1, "morph1 is null.");
        VkBuffer morph0 = renderCmd._pMorph0->getVertexData()->getHandle()->getNativeBuffer();
        VkBuffer morph1 = renderCmd._pMorph1->getVertexData()->getHandle()->getNativeBuffer();
        cmdBuffer->bindVertexBuffers(1, 1, &morph0, offsets);
        cmdBuffer->bindVertexBuffers(2, 1, &morph1, offsets);
      }
  
      cmdBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipe->Pipeline());
      cmdBuffer->setViewPorts(0, 1, &viewport);

      if (debugging) {
        struct ivec4 {
          i32 v[4];
        };
        ivec4 value;
        value.v[0] = renderCmd._debugConfig;
        cmdBuffer->pushConstants(Pipe->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ivec4), &value);
      }

      if (indexBuffer) {
        VkBuffer ib = indexBuffer->getHandle()->getNativeBuffer();
        cmdBuffer->bindIndexBuffer(ib, 0, getNativeIndexType(indexBuffer->GetSizeType()));
      }
      ShadowMapSystem& shadow = m_pLights->getPrimaryShadowMapSystem();
      DescriptorSets[0] = m_pGlobal->getDescriptorSet(frameIndex)->getHandle();
      DescriptorSets[1] = pMeshDesc->getCurrMeshSet(frameIndex)->getHandle();
      DescriptorSets[3] = m_pLights->getDescriptorSet(frameIndex)->getHandle();
      DescriptorSets[4] = shadow.getShadowMapViewDescriptor(frameIndex)->getHandle();
      //DescriptorSets[5] = shadow.StaticShadowMapViewDescriptor(frameIndex)->getHandle();
      DescriptorSets[5] = m_pGlobalIllumination->getDescriptorSet()->getHandle(); // Need global illumination data.
      DescriptorSets[6] = (Skinned ? renderCmd._pJointDesc->getCurrJointSet(frameIndex)->getHandle() : nullptr);

      // Bind materials.
      Primitive* primitive = renderCmd._pPrimitive;
      DescriptorSets[2] = primitive->_pMat->getNative()->CurrMaterialSet()->getHandle();
      cmdBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
        Pipe->getLayout(),
        0,
        (Skinned ? 7 : 6),
        DescriptorSets,
        0,
        nullptr
      );

      if (indexBuffer) {
        cmdBuffer->drawIndexed(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0, 0);
      } else {
        cmdBuffer->draw(primitive->_indexCount, renderCmd._instances, primitive->_firstIndex, 0);
      }
    }
  cmdBuffer->endRenderPass();

  // TODO(): Move particle compute on separate job, can be ran asyncronously!
  m_particleEngine->generateParticleComputeCommands(m_pRhi, cmdBuffer, m_pGlobal, m_particleSystems, frameIndex);
  m_particleEngine->generateParticleRenderCommands(m_pRhi, cmdBuffer, m_pGlobal, m_particleSystems, frameIndex);
}


void Renderer::setUpGlobalIlluminationBuffer()
{
  m_pGlobalIllumination->initialize(m_pRhi, m_currentGraphicsConfigs._EnableLocalReflections);
}


void Renderer::updateGlobalIlluminationBuffer()
{
  Texture* pCube = m_pSky->GetCubeMap();
  Texture* pTex = nullptr;
  if (m_usePreRenderSkybox) {
    pCube = m_preRenderSkybox->getHandle();
  }
  if (m_skybox._brdfLUT) {
    pTex = m_skybox._brdfLUT->getHandle();
  }
  m_pGlobalIllumination->setGlobalProbe(m_globalLightProbe);
  m_pGlobalIllumination->setGlobalEnvMap(pCube);
  m_pGlobalIllumination->setGlobalBRDFLUT(pTex);
  m_pGlobalIllumination->update(this);
}


void Renderer::cleanUpGlobalIlluminationBuffer()
{
  m_pGlobalIllumination->cleanUp(m_pRhi);
}


void Renderer::setUpPBR()
{
  Sampler* pbr_Sampler = gbuffer_SamplerKey;

  DescriptorSetLayout* pbr_Layout = pbr_DescLayoutKey;
  if ( pbr_DescSetKey ) {
    m_pRhi->freeDescriptorSet( pbr_DescSetKey );
    pbr_DescSetKey = nullptr;
  }
  DescriptorSet* pbr_Set = m_pRhi->createDescriptorSet();
  pbr_DescSetKey = pbr_Set;
  {
    VkDescriptorImageInfo albedo = {};
    albedo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    albedo.imageView = gbuffer_AlbedoAttachKey->getView();
    albedo.sampler = pbr_Sampler->getHandle();

    VkDescriptorImageInfo normal = {};
    normal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normal.imageView = gbuffer_NormalAttachKey->getView();
    normal.sampler = pbr_Sampler->getHandle();

    VkDescriptorImageInfo position = {};
    position.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    position.imageView = gbuffer_PositionAttachKey->getView();
    position.sampler = pbr_Sampler->getHandle();

    VkDescriptorImageInfo emission = {};
    emission.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    emission.imageView = gbuffer_EmissionAttachKey->getView();
    emission.sampler = pbr_Sampler->getHandle();

    VkDescriptorImageInfo depth = { };
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    depth.imageView = gbuffer_DepthAttachKey->getView();
    depth.sampler = pbr_Sampler->getHandle();

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
  
    pbr_Set->allocate(m_pRhi->descriptorPool(), pbr_Layout);
    pbr_Set->update(static_cast<u32>(writeInfo.size()), writeInfo.data());
  }

  if (pbr_compSet) {
    m_pRhi->freeDescriptorSet(pbr_compSet);
    pbr_compSet = nullptr;
  }
  pbr_compSet = m_pRhi->createDescriptorSet();
  {
    VkDescriptorImageInfo outResult = { };
    outResult.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    outResult.imageView = pbr_FinalTextureKey->getView();
    outResult.sampler = nullptr;    

    VkDescriptorImageInfo outBright = { };
    outBright.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    outBright.imageView = pbr_BrightTextureKey->getView();
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

    pbr_compSet->allocate(m_pRhi->descriptorPool(), pbr_compDescLayout);
    pbr_compSet->update(static_cast<u32>(writes.size()), writes.data());
  }

  for (u32 i = 0; i < m_Pbr._CmdBuffers.size(); ++i) {
    if (m_Pbr._CmdBuffers[i]) {
      m_pRhi->freeCommandBuffer(m_Pbr._CmdBuffers[i]);
      m_Pbr._CmdBuffers[i] = nullptr;
    }
    if (m_Pbr._Semas[i]) {
      m_pRhi->freeVkSemaphore(m_Pbr._Semas[i]);
      m_Pbr._Semas[i] = nullptr;
    }
  }

  m_Pbr._CmdBuffers.resize(m_pRhi->bufferingCount());
  m_Pbr._Semas.resize(m_pRhi->bufferingCount());

  VkSemaphoreCreateInfo semaCi = {};
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (u32 i = 0; i < m_Pbr._Semas.size(); ++i) {
    m_Pbr._Semas[i] = m_pRhi->createVkSemaphore();
    m_Pbr._Semas[i]->initialize(semaCi);
    m_Pbr._CmdBuffers[i] = m_pRhi->createCommandBuffer();
    m_Pbr._CmdBuffers[i]->allocate(m_pRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  }
}


void Renderer::cleanUpPBR()
{
  DescriptorSet* pbr_Set = pbr_DescSetKey;
  m_pRhi->freeDescriptorSet(pbr_Set);
  m_pRhi->freeDescriptorSet(pbr_compSet);

  pbr_DescSetKey = nullptr;
  pbr_compSet = nullptr;

  for (u32 i = 0; i < m_Pbr._Semas.size(); ++i) {
    m_pRhi->freeVkSemaphore(m_Pbr._Semas[i]);
    m_pRhi->freeCommandBuffer(m_Pbr._CmdBuffers[i]);
    m_Pbr._Semas[i] = nullptr;
    m_Pbr._CmdBuffers[i] = nullptr;
  }
}


void Renderer::setUpFinalOutputs()
{
  Texture* pbr_Final = pbr_FinalTextureKey;
  Texture* hdr_Color = hdr_gamma_colorAttachKey;

  Sampler* hdr_Sampler = hdr_gamma_samplerKey;
  Sampler* pbr_Sampler = gbuffer_SamplerKey;

  DescriptorSetLayout* finalSetLayout = final_DescSetLayoutKey;
  if ( final_DescSetKey ) {
    m_pRhi->freeDescriptorSet( final_DescSetKey );
    final_DescSetKey = nullptr; 
  }
  DescriptorSet* offscreenImageDescriptor = m_pRhi->createDescriptorSet();
  final_DescSetKey = offscreenImageDescriptor;
  offscreenImageDescriptor->allocate(m_pRhi->descriptorPool(), finalSetLayout);
  {
    // Final texture must be either hdr post process texture, or pbr output without hdr.
    VkDescriptorImageInfo renderTextureFinal = {};
    renderTextureFinal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (m_HDR._Enabled) {
      renderTextureFinal.sampler = hdr_Sampler->getHandle();
      renderTextureFinal.imageView = hdr_Color->getView();
    } else {
      renderTextureFinal.sampler = pbr_Sampler->getHandle();
      renderTextureFinal.imageView = pbr_Final->getView();
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

    offscreenImageDescriptor->update(static_cast<u32>(writeInfo.size()), writeInfo.data());
  }

  // Output info
  {
    output_descSetKey = m_pRhi->createDescriptorSet();
    output_descSetKey->allocate(m_pRhi->descriptorPool(), final_DescSetLayoutKey);

    // TODO(): usingAntialiasing will need to be compensated here, similar to final texture, above.
    VkDescriptorImageInfo renderTextureOut = {};
    renderTextureOut.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    renderTextureOut.imageView = final_renderTargetKey->getView();
    renderTextureOut.sampler = hdr_gamma_samplerKey->getHandle();

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
    output_descSetKey->update(static_cast<u32>(writeInfo.size()), writeInfo.data());
  }

  for (u32 i = 0; i < m_pFinalFinishedSemas.size(); ++i) {
    if (m_pFinalFinishedSemas[i]) {
      m_pRhi->freeVkSemaphore(m_pFinalFinishedSemas[i]);
      m_pFinalFinishedSemas[i] = nullptr;
    }
    if (m_pFinalCommandBuffers[i]) {
      m_pRhi->freeCommandBuffer(m_pFinalCommandBuffers[i]);
      m_pFinalCommandBuffers[i] = nullptr;
    }
  }

  m_pFinalFinishedSemas.resize(m_pRhi->bufferingCount());
  m_pFinalCommandBuffers.resize(m_pRhi->bufferingCount());
  VkSemaphoreCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (u32 i = 0; i < m_pFinalFinishedSemas.size(); ++i) {
    m_pFinalFinishedSemas[i] = m_pRhi->createVkSemaphore();
    m_pFinalFinishedSemas[i]->initialize(info); 
    m_pFinalCommandBuffers[i] = m_pRhi->createCommandBuffer();
    m_pFinalCommandBuffers[i]->allocate(m_pRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  }
}


void Renderer::checkCmdUpdate()
{
  R_TIMED_PROFILE_RENDERER();

  i32 frameIndex = m_pRhi->currentFrame();
  VkCommandBufferBeginInfo begin{};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (m_currentGraphicsConfigs._EnableMultithreadedRendering) {
    m_workers[0] = std::thread([&]() -> void {
      CommandBuffer* offscreenCmdList = m_Offscreen._cmdBuffers[frameIndex];
      offscreenCmdList->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      offscreenCmdList->begin(begin);
      generateOffScreenCmds(offscreenCmdList, frameIndex);
      // Should the shadow map be turned off (ex. in night time scenes), we still need to transition
      // it to readable format.
      if (!m_pLights->isPrimaryShadowEnabled()) {
        m_pLights->getPrimaryShadowMapSystem().transitionEmptyShadowMap(offscreenCmdList, frameIndex);
      }
      offscreenCmdList->end();
      
      m_Forward._cmdBuffers[frameIndex]->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

      m_Forward._cmdBuffers[frameIndex]->begin(begin);
      generateForwardPBRCmds(m_Forward._cmdBuffers[frameIndex], frameIndex);
      m_Forward._cmdBuffers[frameIndex]->end();
    });

    if (m_pLights->isPrimaryShadowEnabled() || m_pLights->getPrimaryShadowMapSystem().staticMapNeedsUpdate()) {
      CommandBuffer* shadowBuf = m_Offscreen._shadowCmdBuffers[frameIndex];
      R_ASSERT(shadowBuf, "Shadow Buffer is null.");
      shadowBuf->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      shadowBuf->begin(begin);
      generateShadowCmds(shadowBuf, frameIndex);
      shadowBuf->end();
    }

    m_pUI->BuildCmdBuffers(m_pRhi, m_pGlobal, frameIndex);
    m_workers[0].join();
  } else {
    CommandBuffer* offscreenCmdList = m_Offscreen._cmdBuffers[frameIndex];
    offscreenCmdList->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    offscreenCmdList->begin(begin);
    generateOffScreenCmds(offscreenCmdList, frameIndex);
    // Should the shadow map be turned off (ex. in night time scenes), we still
    // need to transition it to readable format.
    if (!m_pLights->isPrimaryShadowEnabled()) {
      m_pLights->getPrimaryShadowMapSystem().transitionEmptyShadowMap(offscreenCmdList, frameIndex);
    }
    offscreenCmdList->end();

    m_Forward._cmdBuffers[frameIndex]->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    m_Forward._cmdBuffers[frameIndex]->begin(begin);
    generateForwardPBRCmds(m_Forward._cmdBuffers[frameIndex], frameIndex);
    m_Forward._cmdBuffers[frameIndex]->end();

    if (m_pLights->isPrimaryShadowEnabled() || m_pLights->getPrimaryShadowMapSystem().staticMapNeedsUpdate()) {
      CommandBuffer* shadowBuf = m_Offscreen._shadowCmdBuffers[frameIndex];
      R_ASSERT(shadowBuf, "Shadow Buffer is null.");
      shadowBuf->reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      shadowBuf->begin(begin);
      generateShadowCmds(shadowBuf, frameIndex);
      shadowBuf->end();
    }
    m_pUI->BuildCmdBuffers(m_pRhi, m_pGlobal, frameIndex);
  }

#if 0
  if (m_NeedsUpdate) {
   if (m_AsyncBuild) {
      buildAsync();
    } else {
      build();
    }
  }
#endif
}


void Renderer::renderPrimaryShadows()
{

}


void Renderer::cleanUpFinalOutputs()
{
  DescriptorSet* offscreenDescriptorSet = final_DescSetKey;
  m_pRhi->freeDescriptorSet(offscreenDescriptorSet);

  final_DescSetKey = nullptr;
  for (u32 i = 0; i < m_pFinalFinishedSemas.size(); ++i) {
    m_pRhi->freeVkSemaphore(m_pFinalFinishedSemas[i]);
    m_pFinalFinishedSemas[i] = nullptr;
    m_pRhi->freeCommandBuffer(m_pFinalCommandBuffers[i]);
    m_pFinalCommandBuffers[i] = nullptr;
  }
}


void Renderer::updateSky()
{
  m_pSky->MarkDirty();
}


void Renderer::updateLightBuffer(u32 frameIndex)
{
  //R_TIMED_PROFILE_RENDERER();

  LightBuffer* pLights = m_pLights->getData();
  i32 usedPointLights = 0;
  i32 usedSpotLights = 0;
  i32 usedDirectionLights = 0;
  for (; usedPointLights < m_pointLights.Size() && usedPointLights < MAX_POINT_LIGHTS; ++usedPointLights) {
    pLights->_PointLights[usedPointLights] = m_pointLights[usedPointLights];
  }
  
  for (; usedSpotLights < m_spotLights.Size() && usedSpotLights < MAX_SPOT_LIGHTS; ++usedSpotLights) {
    pLights->_SpotLights[usedSpotLights] = m_spotLights[usedSpotLights];
  }

  for (; usedDirectionLights < m_directionalLights.Size() 
          && usedDirectionLights < MAX_DIRECTIONAL_LIGHTS; ++usedDirectionLights) {
    
    pLights->_DirectionalLights[usedDirectionLights] = m_directionalLights[usedDirectionLights];
  }

  while (usedPointLights < LightBuffer::maxNumPointLights()) {
    pLights->_PointLights[usedPointLights] = PointLight();
    ++usedPointLights;
  }

  while (usedSpotLights < LightBuffer::maxNumSpotLights()) {
    pLights->_SpotLights[usedSpotLights] = SpotLight();
    ++usedSpotLights;
  }

  while (usedDirectionLights < LightBuffer::maxNumDirectionalLights()) {
    pLights->_DirectionalLights[usedDirectionLights] = DirectionalLight();
    ++usedDirectionLights;
  }

  m_pLights->checkBuffering(m_pRhi, &m_currentGraphicsConfigs);

  m_pLights->update(m_pRhi, m_pGlobal->getData(), frameIndex);
}


void Renderer::updateSceneDescriptors(u32 frameIndex)
{
  // Update global data.
  m_pGlobal->getData()->_EnableAA = m_AntiAliasing;
  Vector4 vec4 = m_pLights->getData()->_PrimaryLight._Direction;
  Vector4 vAirColor = m_pGlobal->getData()->_vAirColor;
  // Left handed coordinate system, need to flip the z.
  Vector3 sunDir = Vector3(vec4.x, vec4.y, -vec4.z);
  Vector3 airColor = Vector3(vAirColor.x, vAirColor.y, vAirColor.z);
  if (m_pGlobal->getData()->_vSunDir != sunDir && sunDir != Vector3() ||
    m_pSky->GetAirColor() != airColor) {
    m_pGlobal->getData()->_vSunDir = sunDir;
    m_pSky->SetAirColor(airColor);
    m_pSky->MarkDirty();
    m_pLights->getPrimaryShadowMapSystem().signalStaticMapUpdate();
  }

  // Update the global descriptor.
  m_pGlobal->update(m_pRhi, frameIndex);

  // Update lights in scene.
#if 0
  Vector4 vViewerPos = m_pGlobal->getData()->_CameraPos;
  m_pLights->setViewerPosition(Vector3(vViewerPos.x, vViewerPos.y, vViewerPos.z));
#endif
  updateLightBuffer(frameIndex);

  // Update mesh descriptors.
  for (size_t i = 0; i < m_meshDescriptors.Size(); ++i) {
    MeshDescriptor* descriptor = m_meshDescriptors[i];
    descriptor->update(m_pRhi, frameIndex);
  }
  
  // Update material descriptors.
  for (size_t i = 0; i < m_materialDescriptors.Size(); ++i) {
    MaterialDescriptor* descriptor = m_materialDescriptors[i];
    R_ASSERT(descriptor, "Null material descriptor.");
    descriptor->update(m_pRhi);
  }

  // Update Joint descriptors.
  for (size_t i = 0; i < m_jointDescriptors.Size(); ++i) {
    JointDescriptor* descriptor = m_jointDescriptors[i];
    descriptor->update(m_pRhi, frameIndex);
  }

  for (size_t i = 0; i < m_particleSystems.Size(); ++i) {
    ParticleSystem* system = m_particleSystems[i];
    system->update(m_pRhi);
  }

  // Update realtime hdr settings.
  m_pHDR->UpdateToGPU(m_pRhi);
}


void Renderer::renderOverlay()
{
  m_pUI->render(m_pRhi);
}


void Renderer::updateRuntimeConfigs(const GraphicsConfigParams* params)
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
      m_pLights->enablePrimaryShadow(false);
      m_pGlobal->getData()->_EnableShadows = false;
    } break;
    case GRAPHICS_QUALITY_POTATO:
    case GRAPHICS_QUALITY_LOW:
    case GRAPHICS_QUALITY_MEDIUM:
    case GRAPHICS_QUALITY_HIGH:
    case GRAPHICS_QUALITY_ULTRA:
    default:
    {
      m_pLights->enablePrimaryShadow(true);
      m_pGlobal->getData()->_EnableShadows = true;
    } break;
  }

  ShadowMapSystem& sunShadow = m_pLights->getPrimaryShadowMapSystem();
  sunShadow.enableDynamicMapSoftShadows(params->_enableSoftShadows);
  sunShadow.enableStaticMapSoftShadows(params->_enableSoftShadows);

  m_pHDR->getRealtimeConfiguration()->_allowChromaticAberration = 
    (params->_EnableChromaticAberration ? Vector4(1.0f) : Vector4(0.0f));
}


void Renderer::updateRendererConfigs(const GraphicsConfigParams* params)
{
  m_pRhi->deviceWaitIdle();

  if (m_pWindow->getWidth() <= 0 || m_pWindow->getHeight() <= 0) { 
    m_Minimized = true;
    return;
  } else {
    m_Minimized = false;
  }

  VkPresentModeKHR presentMode = m_pRhi->swapchainObject()->CurrentPresentMode();
  u32 bufferCount = m_pRhi->bufferingCount();
  b32 reconstruct = false;

  if (params) {
    switch (params->_Buffering) {
    case SINGLE_BUFFER: { presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; bufferCount = 1; } break;
    case DOUBLE_BUFFER: { presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR; bufferCount = 2; } break;
    case TRIPLE_BUFFER: { presentMode = VK_PRESENT_MODE_MAILBOX_KHR; bufferCount = 3; } break;
    default: presentMode = m_pRhi->swapchainObject()->CurrentPresentMode(); break;
    }

    if (params->_EnableVsync >= 1) {
      presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    if (m_currentGraphicsConfigs._EnableLocalReflections != params->_EnableLocalReflections) {
      reconstruct = true;
    }
    
    if (params->_Resolution != m_currentGraphicsConfigs._Resolution) {
      updateRenderResolution(params->_Resolution);
      reconstruct = true;
    }
    updateRuntimeConfigs(params);

  }

  VkExtent2D extent = { m_renderWidth, m_renderHeight };
  if (!params || (presentMode != m_pRhi->swapchainObject()->CurrentPresentMode()) 
    || (bufferCount != m_pRhi->swapchainObject()->CurrentBufferCount())) {
    reconstruct = true;
  }

  if (reconstruct) {
    m_pGlobal->getData()->_ScreenSize[0] = m_renderWidth;
    m_pGlobal->getData()->_ScreenSize[1] = m_renderHeight;
    // Triple buffering atm, we will need to use user params to switch this.
    m_pRhi->reConfigure(presentMode, m_pWindow->getWidth(), m_pWindow->getHeight(), bufferCount, 3);

    m_pUI->cleanUp(m_pRhi);

    cleanUpForwardPBR();
    cleanUpPBR();
    cleanUpHDR(true);
    cleanUpDownscale(true);
    cleanUpSkybox(true);
    //CleanUpOffscreen();
    cleanUpFinalOutputs();
    cleanUpGraphicsPipelines();
    cleanUpFrameBuffers();
    cleanUpRenderTextures(false);
    m_particleEngine->cleanUpPipeline(m_pRhi);

    setUpRenderTextures(false);
    setUpFrameBuffers();
    setUpGraphicsPipelines();
    setUpForwardPBR();
    m_particleEngine->initializePipeline(m_pRhi);
    m_pUI->initialize(m_pRhi);
  }

  setUpOffscreen();
  setUpDownscale(true);
  setUpHDR(true);
  setUpPBR();
  setUpSkybox(true);
  setUpFinalOutputs();

  cleanUpGlobalIlluminationBuffer();
  setUpGlobalIlluminationBuffer();
  updateGlobalIlluminationBuffer();

  build();
  m_pLights->getPrimaryShadowMapSystem().signalStaticMapUpdate();
}


void Renderer::buildAsync()
{
  static b8 inProgress = false;
  // TODO(): building the command buffers asyncronously requires us
  // to allocate temp commandbuffers, build them, and then swap them with
  // the previous commandbuffers.
  std::thread async([&] () -> void {
    if (inProgress) { return; }

    inProgress = true;
    u32 idx = m_pRhi->currentImageIndex();

    inProgress = false;
  });
}


void Renderer::waitIdle()
{
  m_pRhi->deviceWaitIdle();
}


Texture1D* Renderer::createTexture1D()
{
  Texture1D* texture = new Texture1D();
  return texture;
}


void Renderer::freeTexture1D(Texture1D* texture)
{
  delete texture;
}


Texture2D* Renderer::createTexture2D()
{
  Texture2D* texture = new Texture2D();
  texture->mRhi = m_pRhi;

  return texture;
}


void Renderer::freeTexture2D(Texture2D* texture)
{
  texture->cleanUp();

  delete texture;
}


Texture2DArray* Renderer::createTexture2DArray()
{
  Texture2DArray* texture = new Texture2DArray();
  texture->mRhi = m_pRhi;
  return texture;
}


void Renderer::freeTexture2DArray(Texture2DArray* texture)
{
  if (!texture) return;
  texture->cleanUp();
  delete texture;
}

void Renderer::freeTextureCube(TextureCube* texture)
{
  if (!texture) return;
  texture->cleanUp();
  delete texture;
}


Texture3D* Renderer::createTexture3D()
{
  Texture3D* texture = new Texture3D();
  return texture;
}


MaterialDescriptor* Renderer::createMaterialDescriptor()
{
  MaterialDescriptor* descriptor = new MaterialDescriptor();
  return descriptor;
}


void Renderer::freeMaterialDescriptor(MaterialDescriptor* descriptor)
{
  if (!descriptor) return;
  descriptor->cleanUp(m_pRhi);
  delete descriptor;
}


TextureCube* Renderer::createTextureCube()
{
  TextureCube* pCube = new TextureCube();
  pCube->mRhi = m_pRhi;
  return pCube;
}


MeshDescriptor* Renderer::createMeshDescriptor()
{
  MeshDescriptor* descriptor = new MeshDescriptor();
  return descriptor;
}


JointDescriptor* Renderer::createJointDescriptor()
{
  JointDescriptor* descriptor = new JointDescriptor();
  return descriptor;
}


TextureSampler* Renderer::createTextureSampler(const SamplerInfo& info)
{
  TextureSampler* pSampler = new TextureSampler();
  pSampler->initialize(m_pRhi, info);
  return pSampler;
}


void Renderer::freeJointDescriptor(JointDescriptor* descriptor)
{
  if (!descriptor) return;
  descriptor->cleanUp(m_pRhi);
  delete descriptor;
}


void Renderer::freeTextureSampler(TextureSampler* pSampler)
{
  if (!pSampler) return; 
  pSampler->cleanUp(m_pRhi);
  delete pSampler;
}


void Renderer::freeMeshDescriptor(MeshDescriptor* descriptor)
{
  if (!descriptor) return;
  descriptor->cleanUp(m_pRhi);
  delete descriptor; 
}


void Renderer::enableHDR(b32 enable)
{
  if (m_HDR._Enabled != enable) {
    m_HDR._Enabled = enable;
    updateRendererConfigs(nullptr);
  }
}


const char* Renderer::getDeviceName()
{
  return m_pRhi->deviceName();
}


void Renderer::sortCmdLists()
{
  R_TIMED_PROFILE_RENDERER();

  R_DEBUG(rNotify, "Deferred sort\n")
  m_cmdDeferredList.sort();
  R_DEBUG(rNotify, "Forward sort\n")
  m_forwardCmdList.sort();
  // TODO(): Also sort forward list too.
}


void Renderer::clearCmdLists()
{
  R_TIMED_PROFILE_RENDERER();

  // TODO(): Clear forward command list as well.
  m_cmdDeferredList.clear();
  m_forwardCmdList.clear();
  m_staticCmdList.clear();
  m_dynamicCmdList.clear();
  m_meshDescriptors.clear();
  m_jointDescriptors.clear();
  m_materialDescriptors.clear();
  m_particleSystems.clear();
  m_spotLights.clear();
  m_pointLights.clear();
  m_directionalLights.clear();
}


void Renderer::pushMeshRender(MeshRenderCmd& cmd)
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
      m_materialDescriptors.pushBack(prim._pMat->getNative());
      R_ASSERT(prim._pMat->getNative() == m_materialDescriptors[m_materialDescriptors.Size() - 1], "Corrupted material descriptors.");
    }
    if (primCmd._config & CMD_SKINNED_BIT) {
      R_ASSERT(cmd._pJointDesc, "No joint descriptoer added to this command.");
      m_jointDescriptors.pushBack(cmd._pJointDesc);
    }

    R_ASSERT(cmd._pMeshDesc, "No mesh descriptor added to this command.");
    m_meshDescriptors.pushBack(cmd._pMeshDesc);

    u32 config = primCmd._config;
    if ((config & (CMD_TRANSPARENT_BIT | CMD_TRANSLUCENT_BIT | CMD_FORWARD_BIT | CMD_DEBUG_BIT))) {
      m_forwardCmdList.pushBack(primCmd);
    }
    else {
      m_cmdDeferredList.pushBack(primCmd);
    }

    if ((config & CMD_STATIC_BIT)) {
      m_staticCmdList.pushBack(primCmd);
    }
    else {
      m_dynamicCmdList.pushBack(primCmd);
    }
  }
}


BufferUI* Renderer::getUiBuffer() const
{
  return m_pUI->GetUIBuffer();
}


TextureCube* Renderer::bakeEnvironmentMap(const Vector3& position, u32 texSize)
{
  TextureCube* pTexCube = nullptr;
  if (texSize == 0) return pTexCube;

  pTexCube = new TextureCube();
  Texture* cubeTexture = m_pRhi->createTexture();
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
    cubeTexture->initialize(imageCi, viewCi, false);
  }


    // TODO():
    Matrix4 view;
    Matrix4 proj = Matrix4::perspective(static_cast<r32>(CONST_PI_HALF), 1.0f, 0.1f, 512.0f);
    //proj[1][1] *= -1;
    GlobalBuffer* pGlobal = m_pGlobal->getData();
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
    pGlobal->_InvProj = proj.inverse();
    CommandBuffer cmdBuffer;
    cmdBuffer.SetOwner(m_pRhi->logicDevice()->getNative());
    cmdBuffer.allocate(m_pRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // TODO(): Signal a beginning and end callback or so, when performing 
    // any rendering.
    // Update the scene descriptors before rendering the frame.
    sortCmdLists();

    updateSceneDescriptors(0);

    std::array<Matrix4, 6> viewMatrices = {
      Quaternion::angleAxis(Radians(-90.0f), Vector3(0.0f, 1.0f, 0.0f)).toMatrix4(),
      Quaternion::angleAxis(Radians(90.0f), Vector3(0.0f, 1.0f, 0.0f)).toMatrix4(),

      Quaternion::angleAxis(Radians(90.0f), Vector3(1.0f, 0.0f, 0.0f)).toMatrix4(),
      Quaternion::angleAxis(Radians(-90.0f), Vector3(1.0f, 0.0f, 0.0f)).toMatrix4(),

      Quaternion::angleAxis(Radians(0.0f), Vector3(0.0f, 1.0f, 0.0f)).toMatrix4(),
      Quaternion::angleAxis(Radians(180.0f), Vector3(0.0f, 1.0f, 0.0f)).toMatrix4(),
    };

    // For each cube face.

    // TODO(): Fix view matrix issue, rendering skybox incorrectly!
    for (size_t i = 0; i < 6; ++i) {
      view = Matrix4::translate(Matrix4(), -position) * viewMatrices[i];
      pGlobal->_CameraPos = Vector4(position, 1.0f);
      pGlobal->_View = view;
      pGlobal->_ViewProj = view * proj;
      pGlobal->_InvView = view.inverse();
      pGlobal->_InvViewProj = pGlobal->_ViewProj.inverse();
      m_pGlobal->update(m_pRhi, 0);
      
      VkCommandBufferBeginInfo begin = { };
      begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      cmdBuffer.reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      cmdBuffer.begin(begin);

      // render to frame first.
      generateOffScreenCmds(&cmdBuffer, 0);
      generateShadowCmds(&cmdBuffer, 0);
      generatePbrCmds(&cmdBuffer, 0);
      if (m_pSky->NeedsRendering()) {
        m_pSky->BuildCmdBuffer(m_pRhi, &cmdBuffer);
        m_pSky->MarkClean();
      }
      generateSkyboxCmds(&cmdBuffer, 0);
      generateForwardPBRCmds(&cmdBuffer, 0);

      cmdBuffer.end();

      VkSubmitInfo submit{};
      submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit.commandBufferCount = 1;
      VkCommandBuffer cmdBufs[] = { cmdBuffer.getHandle() };
      submit.pCommandBuffers = cmdBufs;

      m_pRhi->graphicsSubmit(DEFAULT_QUEUE_IDX, 1, &submit, VK_NULL_HANDLE);
      m_pRhi->graphicsWaitIdle(DEFAULT_QUEUE_IDX);

      cmdBuffer.reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      cmdBuffer.begin(begin);
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
      imgMemBarrier.image = cubeTexture->getImage();

      // set the cubemap image layout for transfer from our framebuffer.
      cmdBuffer.pipelineBarrier(
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

      imgMemBarrier.image = pbr_FinalTextureKey->getImage();
      imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imgMemBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      imgMemBarrier.subresourceRange = subRange;

      // transfer color attachment to transfer.
      cmdBuffer.pipelineBarrier(
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

      cmdBuffer.copyImage(
        pbr_FinalTextureKey->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        cubeTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &imgCopy
      );

      imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      imgMemBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      cmdBuffer.pipelineBarrier(
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
      imgMemBarrier.image = cubeTexture->getImage();
      imgMemBarrier.subresourceRange = subRange;

      cmdBuffer.pipelineBarrier(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imgMemBarrier
      );
      cmdBuffer.end();

      submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit.commandBufferCount = 1;
      submit.pCommandBuffers = cmdBufs;

      m_pRhi->graphicsSubmit(DEFAULT_QUEUE_IDX, 1, &submit, VK_NULL_HANDLE);
      m_pRhi->graphicsWaitIdle(DEFAULT_QUEUE_IDX);
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


void Renderer::takeSnapshot(const std::string name)
{
  m_pRhi->waitAllGraphicsQueues();
  Texture2D tex2d;
  tex2d.mRhi = m_pRhi;
  tex2d.texture = final_renderTargetKey;
  tex2d.Save(name);
}


Texture2D* Renderer::generateBRDFLUT(u32 x, u32 y)
{
  Texture2D* tex2D = new Texture2D();
  Texture* texture = m_pRhi->createTexture();
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
    texture->initialize(imgCi, viewCi);
  }

  CommandBuffer cmd;
  cmd.SetOwner(m_pRhi->logicDevice()->getNative());
  cmd.allocate(m_pRhi->computeCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = { };
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmd.begin(beginInfo);

  GlobalBuffer* g = m_pGlobal->getData();
  i32 prevx = g->_ScreenSize[0];
  i32 prevy = g->_ScreenSize[1];
  g->_ScreenSize[0] = x;
  g->_ScreenSize[1] = y;
  m_pGlobal->update(m_pRhi, 0);
  m_pBakeIbl->updateTargetBRDF(texture);
  m_pBakeIbl->renderGenBRDF(&cmd, m_pGlobal, texture, 0);

  cmd.end();

  VkSubmitInfo submitInfo = { };
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer native[] = { cmd.getHandle() };
  submitInfo.pCommandBuffers = native;

  m_pRhi->computeSubmit(0, submitInfo);
  m_pRhi->computeWaitIdle(0);


  g->_ScreenSize[0] = prevx;
  g->_ScreenSize[1] = prevy;
  m_pGlobal->update(m_pRhi, 0);
  tex2D->texture = texture;
  return tex2D;
}


void Renderer::pushParticleSystem(ParticleSystem* system)
{
  if (!system) return;
  m_particleSystems.pushBack(system);
}


ParticleSystem* Renderer::createParticleSystem(u32 maxInitParticleCount)
{
  ParticleSystem* particleSystem = new ParticleSystem();
  particleSystem->initialize(m_pRhi, m_particleEngine->getParticleSystemDescriptorLayout(), maxInitParticleCount);
  particleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT | PARTICLE_DESCRIPTOR_UPDATE_BIT | PARTICLE_VERTEX_BUFFER_UPDATE_BIT);
  return particleSystem;
}


void Renderer::freeParticleSystem(ParticleSystem* particle)
{
  if (!particle) { return; }
  particle->cleanUp(m_pRhi);
  delete particle;
}


UIDescriptor* Renderer::createUIDescriptor()
{
  UIDescriptor* pDesc = new UIDescriptor();
  return pDesc;
}


void Renderer::freeUIDescriptor(UIDescriptor* pDesc)
{
  if (!pDesc) return;
  pDesc->cleanUp(m_pRhi);
  delete pDesc;
}


void Renderer::pushDirectionLight(const DirectionalLight& lightInfo)
{
  m_directionalLights.pushBack(lightInfo);
}


void Renderer::pushPointLight(const PointLight& lightInfo) 
{
  m_pointLights.pushBack(lightInfo);
} 


void Renderer::pushSpotLight(const SpotLight& lightInfo)
{
  m_spotLights.pushBack(lightInfo);
}


void Renderer::pushSimpleRender(SimpleRenderCmd& cmd)
{

}


void Renderer::updateRenderResolution(RenderResolution resolution)
{
  u32 w, h;
  switch ( resolution ) {
    case Resolution_800x600: {
      w = 800;
      h = 600;
    } break;
    case Resolution_1200x800: {
      w = 1200;
      h = 800;
    } break;
    case Resolution_1280x720: {
      w = 1280;
      h = 720;
    } break;
    case Resolution_1440x900: {
      w = 1440;
      h = 900;
    } break;
    case Resolution_1920x1080: {
      w = 1920;
      h = 1080;
    } break;
    case Resolution_1920x1200: {
      w = 1920;
      h = 1200;
    } break;
    case Resolution_2048x1440: {
      w = 2048;
      h = 1440;
    } break;
    case Resolution_3840x2160: {
      w = 3840;
      h = 2160;
    } break;
    case Resolution_7680x4320: {
      w = 7680;
      h = 4320;
    } break;
    default: {
      w = m_pWindow->getWidth();
      h = m_pWindow->getHeight();
    } break;
  }

  m_renderWidth = w;
  m_renderHeight = h;
}
} // Recluse