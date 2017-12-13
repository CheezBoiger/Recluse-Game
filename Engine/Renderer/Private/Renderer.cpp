// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Renderer.hpp"
#include "CmdList.hpp"
#include "Vertex.hpp"
#include "RenderQuad.hpp"
#include "CmdList.hpp"
#include "RenderCmd.hpp"
#include "Material.hpp"
#include "MeshDescriptor.hpp"
#include "UserParams.hpp"
#include "TextureType.hpp"
#include "UIOverlay.hpp"
#include "RendererData.hpp"
#include "RenderObject.hpp"
#include "MeshData.hpp"
#include "LightMaterial.hpp"
#include "GlobalMaterial.hpp"
#include "StructuredBuffer.hpp"
#include "VertexDescription.hpp"

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

#include <array>

namespace Recluse {


Renderer& gRenderer() { 
  return Renderer::Instance();
}


Renderer::Renderer()
  : mRhi(nullptr)
  , mCmdList(nullptr)
  , mDeferredCmdList(nullptr)
  , mRendering(false)
  , mInitialized(false)
{
  mHDR.data.gamma = 2.2f;
  mHDR.data.bloomEnabled = false;
  mHDR.data.exposure = 1.0f;
  mHDR.enabled = true;
  
  mOffscreen.cmdBuffers.resize(2);
  mOffscreen.currCmdBufferIndex = 0;

  mHDR.cmdBuffers.resize(2);
  mOffscreen.currCmdBufferIndex = 0;
  
  m_Downscale.horizontal = 0;
  m_Downscale.strength = 1.0f;
  m_Downscale.scale = 1.0f;
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
  if (!mRhi) mRhi = new VulkanRHI();
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
  mRendering = true;
  mRhi->PresentWaitIdle();
  mRhi->AcquireNextImage();
}


void Renderer::EndFrame()
{
  mRendering = false;
  mRhi->Present();
}


void Renderer::Render()
{
  // TODO(): Signal a beginning and end callback or so, when performing 
  // any rendering.
  VkCommandBuffer offscreenCmd = mOffscreen.cmdBuffers[mOffscreen.currCmdBufferIndex]->Handle();
  VkSemaphore waitSemas[] = { mRhi->SwapchainObject()->ImageAvailableSemaphore() };
  VkSemaphore signalSemas[] = { mOffscreen.semaphore->Handle() };
  VkPipelineStageFlags waitFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

  VkSubmitInfo offscreenSI = {};
  offscreenSI.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  offscreenSI.pCommandBuffers = &offscreenCmd;
  offscreenSI.commandBufferCount = 1;
  offscreenSI.signalSemaphoreCount = 1;
  offscreenSI.pSignalSemaphores = signalSemas;
  offscreenSI.waitSemaphoreCount = 1;
  offscreenSI.pWaitSemaphores = waitSemas;
  offscreenSI.pWaitDstStageMask = waitFlags;

  VkSubmitInfo hdrSI = offscreenSI;
  VkSemaphore hdrWaits[] = { mOffscreen.semaphore->Handle() };
  VkSemaphore hdrSignal[] = { mHDR.semaphore->Handle() };
  VkCommandBuffer hdrCmd = mHDR.cmdBuffers[mHDR.currCmdBufferIndex]->Handle();
  hdrSI.pCommandBuffers = &hdrCmd;
  hdrSI.pSignalSemaphores = hdrSignal;
  hdrSI.pWaitSemaphores = hdrWaits;

  VkSemaphore waitSemaphores = mHDR.semaphore->Handle();
  if (!mHDR.enabled) waitSemaphores = mOffscreen.semaphore->Handle();

  // Update materials before rendering the frame.
  UpdateMaterials();

  // begin frame. This is where we start our render process per frame.
  BeginFrame();
    while (mOffscreen.cmdBuffers[mHDR.currCmdBufferIndex]->Recording() || !mRhi->CmdBuffersComplete()) {}

    // Offscreen PBR Forward Rendering Pass.
    mRhi->GraphicsSubmit(offscreenSI);

    // Offscreen downsampling.
    // mRhi->GraphicsSubmit(downsampleSI);

    // High Dynamic Range and Gamma Pass.
    if (mHDR.enabled) mRhi->GraphicsSubmit(hdrSI);

    // Before calling this cmd buffer, we want to submit our offscreen buffer first, then
    // ssent our signal to our swapchain cmd buffers.
    mRhi->SubmitCurrSwapchainCmdBuffer(1, &waitSemaphores);

    // Render the Overlay.
    RenderOverlay();

  EndFrame();


  // Compute pipeline render.
  VkSubmitInfo computeSubmit = { };
  computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  computeSubmit.commandBufferCount = 0;
  computeSubmit.pCommandBuffers = nullptr;
  computeSubmit.signalSemaphoreCount = 0;
  computeSubmit.waitSemaphoreCount = 0;

  mRhi->ComputeSubmit(computeSubmit);
}


void Renderer::CleanUp()
{
  // Must wait for all command buffers to finish before cleaning up.
  mRhi->DeviceWaitIdle();

  if (mUI) {
    mUI->CleanUp();
    delete mUI;
    mUI = nullptr;
  }

  mRenderQuad.CleanUp();
  CleanUpHDR(true);
  CleanUpDownscale(true);
  CleanUpOffscreen(true);
  CleanUpFinalOutputs();
  CleanUpDescriptorSetLayouts();
  CleanUpGraphicsPipelines();
  CleanUpFrameBuffers();
  CleanUpRenderTextures(true);

  if (mRhi) {
    mRhi->CleanUp();
    delete mRhi;
    mRhi = nullptr;
  }
  mInitialized = false;
}


b8 Renderer::Initialize(Window* window)
{
  if (!window) return false;
  if (mInitialized) return true;
  
  mWindowHandle = window;
  mRhi->Initialize(window->Handle());

  SetUpRenderTextures(true);
  SetUpFrameBuffers();
  SetUpDescriptorSetLayouts();
  SetUpGraphicsPipelines();
  SetUpFinalOutputs();
  SetUpOffscreen(true);
  SetUpDownscale(true);
  SetUpHDR(true);
  mRenderQuad.Initialize(mRhi);

  mRhi->SetSwapchainCmdBufferBuild([&] (CommandBuffer& cmdBuffer, VkRenderPassBeginInfo& defaultRenderpass) -> void {
    // Do stuff with the buffer.
    VkViewport viewport = { };
    viewport.height = (r32) mWindowHandle->Height();
    viewport.width = (r32) mWindowHandle->Width();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    
    GraphicsPipeline* finalPipeline = gResources().GetGraphicsPipeline(FinalPipelineStr);
    DescriptorSet* finalSet = gResources().GetDescriptorSet(FinalDescSetStr);
    
    cmdBuffer.BeginRenderPass(defaultRenderpass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer.SetViewPorts(0, 1, &viewport);
      cmdBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Pipeline());
      VkDescriptorSet finalDescriptorSet = finalSet->Handle();    

      cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Layout(), 0, 1, &finalDescriptorSet, 0, nullptr);
      VkBuffer vertexBuffer = mRenderQuad.Quad()->Handle()->NativeBuffer();
      VkBuffer indexBuffer = mRenderQuad.Indices()->Handle()->NativeBuffer();
      VkDeviceSize offsets[] = { 0 };

      cmdBuffer.BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer.BindVertexBuffers(0, 1, &vertexBuffer, offsets);

      cmdBuffer.DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer.EndRenderPass();
  });

  if (!mUI) {
    mUI = new UIOverlay();
    mUI->Initialize(mRhi);
  }

  mInitialized = true;
  return true;
}


void Renderer::SetUpDescriptorSetLayouts()
{
  std::array<VkDescriptorSetLayoutBinding, 1> objLayoutBindings;
  objLayoutBindings[0].binding = 0;
  objLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  objLayoutBindings[0].descriptorCount = 1;
  objLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  objLayoutBindings[0].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layout0 = {};
  layout0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout0.bindingCount = static_cast<u32>(objLayoutBindings.size());
  layout0.pBindings = objLayoutBindings.data();

  DescriptorSetLayout* d0 = mRhi->CreateDescriptorSetLayout();
  d0->Initialize(layout0);

  std::vector<VkDescriptorSetLayoutBinding> bindings(8);
  bindings[0].binding = 0;
  bindings[0].descriptorCount = 1;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].pImmutableSamplers = nullptr;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  // albedo
  bindings[1].binding = 2;
  bindings[1].descriptorCount = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].pImmutableSamplers = nullptr;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // metallic
  bindings[2].binding = 3;
  bindings[2].descriptorCount = 1;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[2].pImmutableSamplers = nullptr;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // roughness
  bindings[3].binding = 4;
  bindings[3].descriptorCount = 1;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[3].pImmutableSamplers = nullptr;
  bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // normal
  bindings[4].binding = 5;
  bindings[4].descriptorCount = 1;
  bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[4].pImmutableSamplers = nullptr;
  bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // ao
  bindings[5].binding = 6;
  bindings[5].descriptorCount = 1;
  bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[5].pImmutableSamplers = nullptr;
  bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // emissive
  bindings[6].binding = 7;
  bindings[6].descriptorCount = 1;
  bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[6].pImmutableSamplers = nullptr;
  bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[7].binding = 1;
  bindings[7].descriptorCount = 1;
  bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[7].pImmutableSamplers = nullptr;
  bindings[7].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


  VkDescriptorSetLayoutCreateInfo layout1 = {};
  layout1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout1.bindingCount = 8;
  layout1.pBindings = bindings.data();

  DescriptorSetLayout* d1 = mRhi->CreateDescriptorSetLayout();
  d1->Initialize(layout1);

  std::array<VkDescriptorSetLayoutBinding, 2> lightLayoutBindings;
  lightLayoutBindings[0].binding = 0;
  lightLayoutBindings[0].descriptorCount = 1;
  lightLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lightLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  lightLayoutBindings[0].pImmutableSamplers = nullptr;

  lightLayoutBindings[1].binding = 1;
  lightLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  lightLayoutBindings[1].descriptorCount = 1;
  lightLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  lightLayoutBindings[1].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layout2 = {};
  layout2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout2.bindingCount = static_cast<u32>(lightLayoutBindings.size());
  layout2.pBindings = lightLayoutBindings.data();

  DescriptorSetLayout* d2 = mRhi->CreateDescriptorSetLayout();
  d2->Initialize(layout2);

  gResources().RegisterDescriptorSetLayout(PBRGlobalMatLayoutStr, d0);
  gResources().RegisterDescriptorSetLayout(PBRObjMatLayoutStr, d1);
  gResources().RegisterDescriptorSetLayout(PBRLightMatLayoutStr, d2);

  DescriptorSetLayout* finalSetLayout = mRhi->CreateDescriptorSetLayout();
  gResources().RegisterDescriptorSetLayout(FinalDescSetLayoutStr, finalSetLayout);

  VkDescriptorSetLayoutBinding finalTextureSample = {};
  finalTextureSample.binding = 0;
  finalTextureSample.descriptorCount = 1;
  finalTextureSample.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  finalTextureSample.pImmutableSamplers = nullptr;
  finalTextureSample.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo finalLayoutInfo = {};
  finalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  finalLayoutInfo.bindingCount = 1;
  finalLayoutInfo.pBindings = &finalTextureSample;

  finalSetLayout->Initialize(finalLayoutInfo);

  // HDR Layout pass.
  DescriptorSetLayout* hdrSetLayout = mRhi->CreateDescriptorSetLayout();
  gResources().RegisterDescriptorSetLayout(HDRGammaDescSetLayoutStr, hdrSetLayout);
  VkDescriptorSetLayoutBinding hdrBindings[3];
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
  hdrLayoutCi.bindingCount = 3;
  hdrLayoutCi.pBindings = hdrBindings;
  
  hdrSetLayout->Initialize(hdrLayoutCi);

  // Downscale descriptor set layout info.
  DescriptorSetLayout* downscaleLayout = mRhi->CreateDescriptorSetLayout();
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

  DescriptorSetLayout* GlowLayout = mRhi->CreateDescriptorSetLayout();
  gResources().RegisterDescriptorSetLayout(GlowDescriptorSetLayoutStr, GlowLayout);
  VkDescriptorSetLayoutBinding glow[3];

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

  dwnLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dwnLayout.bindingCount = 3;
  dwnLayout.pBindings = glow;
  dwnLayout.flags = 0;
  dwnLayout.pNext = nullptr;

  GlowLayout->Initialize(dwnLayout);
}


void Renderer::CleanUpDescriptorSetLayouts()
{
  DescriptorSetLayout* d0 = gResources().UnregisterDescriptorSetLayout(PBRGlobalMatLayoutStr);
  DescriptorSetLayout* d1 = gResources().UnregisterDescriptorSetLayout(PBRObjMatLayoutStr);
  DescriptorSetLayout* d2 = gResources().UnregisterDescriptorSetLayout(PBRLightMatLayoutStr);

  mRhi->FreeDescriptorSetLayout(d0);
  mRhi->FreeDescriptorSetLayout(d1);
  mRhi->FreeDescriptorSetLayout(d2);

  DescriptorSetLayout* finalSetLayout = gResources().UnregisterDescriptorSetLayout(FinalDescSetLayoutStr);
  mRhi->FreeDescriptorSetLayout(finalSetLayout);

  DescriptorSetLayout* hdrSetLayout = gResources().UnregisterDescriptorSetLayout(HDRGammaDescSetLayoutStr);
  mRhi->FreeDescriptorSetLayout(hdrSetLayout);

  DescriptorSetLayout* downscaleLayout = gResources().UnregisterDescriptorSetLayout(DownscaleBlurLayoutStr);
  mRhi->FreeDescriptorSetLayout(downscaleLayout);

  DescriptorSetLayout* GlowLayout = gResources().UnregisterDescriptorSetLayout(GlowDescriptorSetLayoutStr);
  mRhi->FreeDescriptorSetLayout(GlowLayout);
}


void Renderer::SetUpFrameBuffers()
{
  Texture* pbrColor = gResources().GetRenderTexture(PBRColorAttachStr);
  Texture* pbrNormal = gResources().GetRenderTexture(PBRNormalAttachStr);
  Texture* pbrDepth = gResources().GetRenderTexture(PBRDepthAttachStr);
  Texture* RTBright = gResources().GetRenderTexture(RenderTargetBrightStr);

  FrameBuffer* pbrFrameBuffer = mRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer(PBRFrameBufferStr, pbrFrameBuffer);

  FrameBuffer* hdrFrameBuffer = mRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer(HDRGammaFrameBufferStr, hdrFrameBuffer);

  std::array<VkAttachmentDescription, 4> attachmentDescriptions;
  VkSubpassDependency dependencies[2];

  attachmentDescriptions[0] = CreateAttachmentDescription(
    pbrColor->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    pbrColor->Samples()
  );

  attachmentDescriptions[1] = CreateAttachmentDescription(
    pbrNormal->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    pbrNormal->Samples()
  );

  attachmentDescriptions[2] = CreateAttachmentDescription(
    RTBright->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    RTBright->Samples()
  );

  attachmentDescriptions[3] = CreateAttachmentDescription(
    pbrDepth->Format(),
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE,
    pbrDepth->Samples()
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

  std::array<VkAttachmentReference, 3> attachmentColors;
  VkAttachmentReference attachmentDepthRef = { static_cast<u32>(attachmentColors.size()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
  attachmentColors[0].attachment = 0;
  attachmentColors[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[1].attachment = 1;
  attachmentColors[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachmentColors[2].attachment = 2;
  attachmentColors[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

  std::array<VkImageView, 4> attachments;
  attachments[0] = pbrColor->View();
  attachments[1] = pbrNormal->View();
  attachments[2] = RTBright->View();
  attachments[3] = pbrDepth->View();

  VkFramebufferCreateInfo framebufferCI = CreateFrameBufferInfo(
    mWindowHandle->Width(),
    mWindowHandle->Height(),
    nullptr, // Finalize() call handles this for us.
    static_cast<u32>(attachments.size()),
    attachments.data(),
    1
  );

  pbrFrameBuffer->Finalize(framebufferCI, renderpassCI);
  
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
  Texture* GlowTarget = gResources().GetRenderTexture(RenderTargetGlowStr);

  FrameBuffer* DownScaleFB2x = mRhi->CreateFrameBuffer();
  FrameBuffer* FB2xFinal = mRhi->CreateFrameBuffer();
  FrameBuffer* DownScaleFB4x = mRhi->CreateFrameBuffer();
  FrameBuffer* FB4xFinal = mRhi->CreateFrameBuffer();
  FrameBuffer* DownScaleFB8x = mRhi->CreateFrameBuffer();
  FrameBuffer* FB8xFinal = mRhi->CreateFrameBuffer();
  FrameBuffer* GlowFB = mRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer(FrameBuffer2xHorizStr, DownScaleFB2x);
  gResources().RegisterFrameBuffer(FrameBuffer2xFinalStr, FB2xFinal);
  gResources().RegisterFrameBuffer(FrameBuffer4xStr, DownScaleFB4x); 
  gResources().RegisterFrameBuffer(FrameBuffer4xFinalStr, FB4xFinal);
  gResources().RegisterFrameBuffer(FrameBuffer8xStr, DownScaleFB8x);
  gResources().RegisterFrameBuffer(FrameBuffer8xFinalStr, FB8xFinal);
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

  // Glow
  attachments[0] = GlowTarget->View();
  attachmentDescriptions[0].format = GlowTarget->Format();
  attachmentDescriptions[0].samples = GlowTarget->Samples();
  framebufferCI.width = mRhi->SwapchainObject()->SwapchainExtent().width;
  framebufferCI.height = mRhi->SwapchainObject()->SwapchainExtent().height;
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
  viewport.height = static_cast<r32>(mRhi->SwapchainObject()->SwapchainExtent().height);
  viewport.width = static_cast<r32>(mRhi->SwapchainObject()->SwapchainExtent().width);

  VkRect2D scissor = { };
  scissor.extent = mRhi->SwapchainObject()->SwapchainExtent();
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
      VK_CULL_MODE_NONE,
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

  std::array<VkPipelineColorBlendAttachmentState, 3> colorBlendAttachments;
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


  VkPipelineColorBlendStateCreateInfo colorBlendCI = CreateBlendStateInfo(
    static_cast<u32>(colorBlendAttachments.size()),
    colorBlendAttachments.data(),
    VK_FALSE,
    VK_LOGIC_OP_NO_OP
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
    
  RendererPass::SetUpPBRForwardPass(RHI(), Filepath, GraphicsPipelineInfo);

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
  RendererPass::SetUpDownScalePass(RHI(), Filepath, GraphicsPipelineInfo);
  RendererPass::SetUpHDRGammaPass(RHI(), Filepath, GraphicsPipelineInfo);
  colorBlendAttachments[0].blendEnable = VK_FALSE;
  RendererPass::SetUpFinalPass(RHI(), Filepath, GraphicsPipelineInfo);
  RendererPass::SetUpShadowPass(RHI(), Filepath, GraphicsPipelineInfo);
}


void Renderer::CleanUpGraphicsPipelines()
{
  GraphicsPipeline* PbrPipeline = gResources().UnregisterGraphicsPipeline(PBRPipelineStr);
  mRhi->FreeGraphicsPipeline(PbrPipeline);

  GraphicsPipeline* PbrStaticPipeline = gResources().UnregisterGraphicsPipeline(PBRStaticPipelineStr);
  mRhi->FreeGraphicsPipeline(PbrStaticPipeline);

  GraphicsPipeline* QuadPipeline = gResources().UnregisterGraphicsPipeline(FinalPipelineStr);
  mRhi->FreeGraphicsPipeline(QuadPipeline);

  GraphicsPipeline* HdrPipeline = gResources().UnregisterGraphicsPipeline(HDRGammaPipelineStr);
  mRhi->FreeGraphicsPipeline(HdrPipeline);

  GraphicsPipeline* DownscalePipeline2x = gResources().UnregisterGraphicsPipeline(DownscaleBlurPipeline2xStr);
  mRhi->FreeGraphicsPipeline(DownscalePipeline2x);

  GraphicsPipeline* DownscalePipeline4x = gResources().UnregisterGraphicsPipeline(DownscaleBlurPipeline4xStr);
  mRhi->FreeGraphicsPipeline(DownscalePipeline4x);

  GraphicsPipeline* DownscalePipeline8x = gResources().UnregisterGraphicsPipeline(DownscaleBlurPipeline8xStr);
  mRhi->FreeGraphicsPipeline(DownscalePipeline8x);

  GraphicsPipeline* GlowPipeline = gResources().UnregisterGraphicsPipeline(GlowPipelineStr);
  mRhi->FreeGraphicsPipeline(GlowPipeline);
}


void Renderer::CleanUpFrameBuffers()
{
  FrameBuffer* pbrFrameBuffer = gResources().UnregisterFrameBuffer(PBRFrameBufferStr);
  mRhi->FreeFrameBuffer(pbrFrameBuffer);

  FrameBuffer* hdrFrameBuffer = gResources().UnregisterFrameBuffer(HDRGammaFrameBufferStr);
  mRhi->FreeFrameBuffer(hdrFrameBuffer);

  FrameBuffer* DownScaleFB2x = gResources().UnregisterFrameBuffer(FrameBuffer2xHorizStr);
  FrameBuffer* FB2xFinal = gResources().UnregisterFrameBuffer(FrameBuffer2xFinalStr);
  FrameBuffer* DownScaleFB4x = gResources().UnregisterFrameBuffer(FrameBuffer4xStr);
  FrameBuffer* FB4xFinal = gResources().UnregisterFrameBuffer(FrameBuffer4xFinalStr);
  FrameBuffer* DownScaleFB8x = gResources().UnregisterFrameBuffer(FrameBuffer8xStr);
  FrameBuffer* FB8xFinal = gResources().UnregisterFrameBuffer(FrameBuffer8xFinalStr);
  FrameBuffer* GlowFB = gResources().UnregisterFrameBuffer(FrameBufferGlowStr);

  mRhi->FreeFrameBuffer(DownScaleFB2x);
  mRhi->FreeFrameBuffer(DownScaleFB4x);
  mRhi->FreeFrameBuffer(DownScaleFB8x);
  mRhi->FreeFrameBuffer(FB2xFinal);
  mRhi->FreeFrameBuffer(FB4xFinal);
  mRhi->FreeFrameBuffer(FB8xFinal);
  mRhi->FreeFrameBuffer(GlowFB);
}


void Renderer::SetUpRenderTextures(b8 fullSetup)
{
  Texture* renderTarget2xScaled = mRhi->CreateTexture();
  Texture* RenderTarget2xFinal = mRhi->CreateTexture();
  Texture* renderTarget4xScaled = mRhi->CreateTexture();
  Texture* RenderTarget4xFinal = mRhi->CreateTexture();
  Texture* renderTarget8xScaled = mRhi->CreateTexture();
  Texture* RenderTarget8xFinal = mRhi->CreateTexture();
  Texture* RenderTargetBright = mRhi->CreateTexture();
  Texture* GlowTarget = mRhi->CreateTexture();

  Texture* pbrColor = mRhi->CreateTexture();
  Texture* pbrNormal = mRhi->CreateTexture();
  Texture* pbrDepth = mRhi->CreateTexture();
  Sampler* pbrSampler = mRhi->CreateSampler();
  Texture* hdrTexture = mRhi->CreateTexture();
  Sampler* hdrSampler = mRhi->CreateSampler();

  gResources().RegisterSampler(HDRGammaSamplerStr, hdrSampler);
  gResources().RegisterRenderTexture(HDRGammaColorAttachStr, hdrTexture);
  gResources().RegisterRenderTexture(PBRColorAttachStr, pbrColor);
  gResources().RegisterRenderTexture(PBRNormalAttachStr, pbrNormal);
  gResources().RegisterRenderTexture(PBRDepthAttachStr, pbrDepth);
  gResources().RegisterRenderTexture(RenderTargetBrightStr, RenderTargetBright);
  gResources().RegisterRenderTexture(RenderTarget2xHorizStr, renderTarget2xScaled);
  gResources().RegisterRenderTexture(RenderTarget2xFinalStr, RenderTarget2xFinal);
  gResources().RegisterRenderTexture(RenderTarget4xScaledStr, renderTarget4xScaled);
  gResources().RegisterRenderTexture(RenderTarget4xFinalStr, RenderTarget4xFinal);
  gResources().RegisterRenderTexture(RenderTarget8xScaledStr, renderTarget8xScaled);
  gResources().RegisterRenderTexture(RenderTarget8xFinalStr, RenderTarget8xFinal);
  gResources().RegisterRenderTexture(RenderTargetGlowStr, GlowTarget);
  gResources().RegisterSampler(PBRSamplerStr, pbrSampler);
  
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
  cImageInfo.extent.width = mWindowHandle->Width();
  cImageInfo.extent.height = mWindowHandle->Height();
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

  pbrColor->Initialize(cImageInfo, cViewInfo);
  pbrNormal->Initialize(cImageInfo, cViewInfo);
  RenderTargetBright->Initialize(cImageInfo, cViewInfo);
  GlowTarget->Initialize(cImageInfo, cViewInfo);

  // Initialize downscaled render textures.
  cImageInfo.extent.width = mWindowHandle->Width()    >> 1;
  cImageInfo.extent.height = mWindowHandle->Height()  >> 1;
  renderTarget2xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget2xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width = mWindowHandle->Width()    >> 2;
  cImageInfo.extent.height = mWindowHandle->Height()  >> 2;
  renderTarget4xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget4xFinal->Initialize(cImageInfo, cViewInfo);

  cImageInfo.extent.width =  mWindowHandle->Width()   >> 3;
  cImageInfo.extent.height = mWindowHandle->Height()  >> 3;
  renderTarget8xScaled->Initialize(cImageInfo, cViewInfo);
  RenderTarget8xFinal->Initialize(cImageInfo, cViewInfo);

  // Depth attachment texture.
  cImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  cImageInfo.extent.width = mWindowHandle->Width();
  cImageInfo.extent.height = mWindowHandle->Height();
  cViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  hdrTexture->Initialize(cImageInfo, cViewInfo);

  cImageInfo.usage = mRhi->DepthUsageFlags() | VK_IMAGE_USAGE_SAMPLED_BIT;
  cImageInfo.format = mRhi->DepthFormat();

  cViewInfo.format = mRhi->DepthFormat();
  cViewInfo.subresourceRange.aspectMask = mRhi->DepthAspectFlags();

  pbrDepth->Initialize(cImageInfo, cViewInfo);

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

  pbrSampler->Initialize(samplerCI);
  hdrSampler->Initialize(samplerCI);
  if (fullSetup) {
    Sampler* defaultSampler = mRhi->CreateSampler();
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
    dImageInfo.extent.width = mWindowHandle->Width();
    dImageInfo.extent.height = mWindowHandle->Height();
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

    Texture* defaultTexture = mRhi->CreateTexture();

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
  Texture* GlowTarget = gResources().UnregisterRenderTexture(RenderTargetGlowStr);

  mRhi->FreeTexture(renderTarget2xScaled);
  mRhi->FreeTexture(RenderTarget2xFinal);
  mRhi->FreeTexture(renderTarget4xScaled);
  mRhi->FreeTexture(RenderTarget4xFinal);
  mRhi->FreeTexture(RenderTarget8xFinal);
  mRhi->FreeTexture(renderTarget8xScaled);
  mRhi->FreeTexture(GlowTarget);

  Texture* pbrColor = gResources().UnregisterRenderTexture(PBRColorAttachStr);
  Texture* pbrNormal = gResources().UnregisterRenderTexture(PBRNormalAttachStr);
  Texture* pbrDepth = gResources().UnregisterRenderTexture(PBRDepthAttachStr);
  Texture* RenderTargetBright = gResources().UnregisterRenderTexture(RenderTargetBrightStr);
  Sampler* pbrSampler = gResources().UnregisterSampler(PBRSamplerStr);

  Texture* hdrTexture = gResources().UnregisterRenderTexture(HDRGammaColorAttachStr);
  Sampler* hdrSampler = gResources().UnregisterSampler(HDRGammaSamplerStr);
  
  mRhi->FreeTexture(hdrTexture);
  mRhi->FreeSampler(hdrSampler);

  mRhi->FreeTexture(pbrColor);
  mRhi->FreeTexture(pbrNormal);
  mRhi->FreeTexture(pbrDepth);
  mRhi->FreeTexture(RenderTargetBright);
  mRhi->FreeSampler(pbrSampler);

  if (fullCleanup) {
    Texture* defaultTexture = gResources().UnregisterRenderTexture(DefaultTextureStr);
    Sampler* defaultSampler = gResources().UnregisterSampler(DefaultSamplerStr);

    mRhi->FreeTexture(defaultTexture);
    mRhi->FreeSampler(defaultSampler);
  }
}


void Renderer::SetUpOffscreen(b8 fullSetup)
{
  if (fullSetup) {
    mOffscreen.semaphore = mRhi->CreateVkSemaphore();
    VkSemaphoreCreateInfo semaCI = { };
    semaCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    mOffscreen.semaphore->Initialize(semaCI);

    for (size_t i = 0; i < mOffscreen.cmdBuffers.size(); ++i) {
      mOffscreen.cmdBuffers[i] = mRhi->CreateCommandBuffer();
      mOffscreen.cmdBuffers[i]->Allocate(mRhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
  }
}


void Renderer::CleanUpOffscreen(b8 fullCleanup)
{
  if (fullCleanup) {
    mRhi->FreeVkSemaphore(mOffscreen.semaphore);
    for (size_t i = 0; i < mOffscreen.cmdBuffers.size(); ++i) {
      mRhi->FreeCommandBuffer(mOffscreen.cmdBuffers[i]);
    }
  }
}


void Renderer::SetUpDownscale(b8 FullSetUp)
{
  if (FullSetUp) {
  }

  DescriptorSetLayout* Layout = gResources().GetDescriptorSetLayout(DownscaleBlurLayoutStr);
  DescriptorSetLayout* GlowLayout = gResources().GetDescriptorSetLayout(GlowDescriptorSetLayoutStr);
  DescriptorSet* DBDS2x = mRhi->CreateDescriptorSet();
  DescriptorSet* DBDS2xFinal = mRhi->CreateDescriptorSet();
  DescriptorSet* DBDS4x = mRhi->CreateDescriptorSet();
  DescriptorSet* DBDS4xFinal = mRhi->CreateDescriptorSet();
  DescriptorSet* DBDS8x = mRhi->CreateDescriptorSet();
  DescriptorSet* DBDS8xFinal = mRhi->CreateDescriptorSet();
  DescriptorSet* GlowDS = mRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet2x, DBDS2x);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet4x, DBDS4x);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet8x, DBDS8x);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet2xFinalStr, DBDS2xFinal);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet4xFinalStr, DBDS4xFinal);
  gResources().RegisterDescriptorSet(DownscaleBlurDescriptorSet8xFinalStr, DBDS8xFinal);
  gResources().RegisterDescriptorSet(GlowDescriptorSetStr, GlowDS);

  DBDS2x->Allocate(mRhi->DescriptorPool(), Layout);
  DBDS4x->Allocate(mRhi->DescriptorPool(), Layout);
  DBDS8x->Allocate(mRhi->DescriptorPool(), Layout);
  DBDS2xFinal->Allocate(mRhi->DescriptorPool(), Layout);
  DBDS4xFinal->Allocate(mRhi->DescriptorPool(), Layout);
  DBDS8xFinal->Allocate(mRhi->DescriptorPool(), Layout);
  GlowDS->Allocate(mRhi->DescriptorPool(), GlowLayout);

  Texture* PBRColor = gResources().GetRenderTexture(PBRColorAttachStr);
  Texture* RTBright = gResources().GetRenderTexture(RenderTargetBrightStr);
  Texture* Color2x = gResources().GetRenderTexture(RenderTarget2xHorizStr);
  Texture* Color2xFinal = gResources().GetRenderTexture(RenderTarget2xFinalStr);
  Texture* Color4x = gResources().GetRenderTexture(RenderTarget4xScaledStr);
  Texture* Color4xFinal = gResources().GetRenderTexture(RenderTarget4xFinalStr);
  Texture* Color8x = gResources().GetRenderTexture(RenderTarget8xScaledStr);
  Texture* Color8xFinal = gResources().GetRenderTexture(RenderTarget8xFinalStr);
  Sampler* PBRSampler = gResources().GetSampler(PBRSamplerStr);
  Sampler* DownscaleSampler = gResources().GetSampler(ScaledSamplerStr);

  VkDescriptorImageInfo Img = { };
  Img.sampler = PBRSampler->Handle();
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

  Img.imageView = Color2xFinal->View();

  VkDescriptorImageInfo Img1 = { };
  Img1.sampler = PBRSampler->Handle();
  Img1.imageView = Color4xFinal->View();
  Img1.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkDescriptorImageInfo Img2 = { };
  Img2.sampler = PBRSampler->Handle();
  Img2.imageView = Color8xFinal->View();
  Img2.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Glow buffer.
  VkWriteDescriptorSet GlowWrites[3];
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

  GlowDS->Update(3, GlowWrites);
}


void Renderer::CleanUpDownscale(b8 FullCleanUp)
{
  if (FullCleanUp) {
  }

  DescriptorSet* DBDS2x = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet2x);
  mRhi->FreeDescriptorSet(DBDS2x);
  DescriptorSet* DBDS4x = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet4x);
  mRhi->FreeDescriptorSet(DBDS4x);
  DescriptorSet* DBDS8x = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet8x);
  mRhi->FreeDescriptorSet(DBDS8x);
  DescriptorSet* DBDS2xFinal = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet2xFinalStr);
  mRhi->FreeDescriptorSet(DBDS2xFinal);
  DescriptorSet* DBDS4xFinal = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet4xFinalStr);
  mRhi->FreeDescriptorSet(DBDS4xFinal);
  DescriptorSet* DBDS8xFinal = gResources().UnregisterDescriptorSet(DownscaleBlurDescriptorSet8xFinalStr);
  mRhi->FreeDescriptorSet(DBDS8xFinal);
  DescriptorSet* GlowDS = gResources().UnregisterDescriptorSet(GlowDescriptorSetStr);
  mRhi->FreeDescriptorSet(GlowDS);
}


void Renderer::SetUpHDR(b8 fullSetUp)
{
  if (fullSetUp) {
    mHDR.hdrBuffer = mRhi->CreateBuffer();
    VkBufferCreateInfo bufferCi = { };
    bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCi.size = sizeof(mHDR.data);
    bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    mHDR.hdrBuffer->Initialize(bufferCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mHDR.hdrBuffer->Map();
      memcpy(mHDR.hdrBuffer->Mapped(), &mHDR.data, sizeof(mHDR.data));
    mHDR.hdrBuffer->UnMap();
 
    for (size_t i = 0; i < mHDR.cmdBuffers.size(); ++i) {
      mHDR.cmdBuffers[i] = mRhi->CreateCommandBuffer();
      mHDR.cmdBuffers[i]->Allocate(mRhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
    mHDR.semaphore = mRhi->CreateVkSemaphore();
    VkSemaphoreCreateInfo semaCi = { };
    semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    mHDR.semaphore->Initialize(semaCi);
  }

  DescriptorSet* hdrSet = mRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet(HDRGammaDescSetStr, hdrSet);
  VkWriteDescriptorSet hdrWrites[3];
  VkDescriptorBufferInfo hdrBufferInfo = {};
  hdrBufferInfo.offset = 0;
  hdrBufferInfo.range = sizeof(mHDR.data);
  hdrBufferInfo.buffer = mHDR.hdrBuffer->NativeBuffer();

  VkDescriptorImageInfo pbrImageInfo = { };
  pbrImageInfo.sampler = gResources().GetSampler(PBRSamplerStr)->Handle();
  pbrImageInfo.imageView = gResources().GetRenderTexture(PBRColorAttachStr)->View();
  pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // TODO(): We don't have our bloom pipeline and texture yet, we will sub it with this instead!
  VkDescriptorImageInfo bloomImageInfo = { };
  bloomImageInfo.sampler = gResources().GetSampler(PBRSamplerStr)->Handle();
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
  hdrSet->Allocate(mRhi->DescriptorPool(), gResources().GetDescriptorSetLayout(HDRGammaDescSetLayoutStr));
  hdrSet->Update(3, hdrWrites);
}


void Renderer::CleanUpHDR(b8 fullCleanUp)
{
  if (fullCleanUp) {
    mRhi->FreeBuffer(mHDR.hdrBuffer);
    mRhi->FreeVkSemaphore(mHDR.semaphore);

    for (size_t i = 0; i < mHDR.cmdBuffers.size(); ++i) {
      mRhi->FreeCommandBuffer(mHDR.cmdBuffers[i]);
      mHDR.cmdBuffers[i] = nullptr;
    }

    mHDR.semaphore = nullptr;
    mHDR.hdrBuffer = nullptr;
  }

  DescriptorSet* hdrSet = gResources().UnregisterDescriptorSet(HDRGammaDescSetStr);
  mRhi->FreeDescriptorSet(hdrSet);
}


void Renderer::Build()
{
  mRhi->GraphicsWaitIdle();

  BuildOffScreenBuffer(mOffscreen.currCmdBufferIndex);
  BuildHDRCmdBuffer(mHDR.currCmdBufferIndex);
  mRhi->RebuildCommandBuffers(mRhi->CurrentSwapchainCmdBufferSet());
}


void Renderer::BuildOffScreenBuffer(u32 cmdBufferIndex)
{
  if (cmdBufferIndex >= mOffscreen.cmdBuffers.size()) { 
    R_DEBUG(rError, "Attempted to build offscreen cmd buffer. Index out of bounds!\n");
    return; 
  }

  if (!mLightMat || !mGlobalMat) {  
    Log(rWarning) << "Can not build commandbuffers without light or global data! One of them is null!";
  } 

  CommandBuffer* cmdBuffer = mOffscreen.cmdBuffers[cmdBufferIndex];
  FrameBuffer* pbrBuffer = gResources().GetFrameBuffer(PBRFrameBufferStr);
  GraphicsPipeline* pbrPipeline = gResources().GetGraphicsPipeline(PBRPipelineStr);
  GraphicsPipeline* staticPbrPipeline = gResources().GetGraphicsPipeline(PBRStaticPipelineStr);

  if (cmdBuffer && !cmdBuffer->Recording()) {

    mRhi->DeviceWaitIdle();
    cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  }

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  std::array<VkClearValue, 4> clearValues;
  clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
  clearValues[3].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo pbrRenderPassInfo = {};
  pbrRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  pbrRenderPassInfo.framebuffer = pbrBuffer->Handle();
  pbrRenderPassInfo.renderPass = pbrBuffer->RenderPass();
  pbrRenderPassInfo.pClearValues = clearValues.data();
  pbrRenderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
  pbrRenderPassInfo.renderArea.extent = mRhi->SwapchainObject()->SwapchainExtent();
  pbrRenderPassInfo.renderArea.offset = { 0, 0 };

  VkViewport viewport =  { };
  viewport.height = (r32)mWindowHandle->Height();
  viewport.width = (r32)mWindowHandle->Width();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  cmdBuffer->Begin(beginInfo);
    cmdBuffer->BeginRenderPass(pbrRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    if (mCmdList) {
      for (size_t i = 0; i < mCmdList->Size(); ++i) {
        RenderCmd& renderCmd = mCmdList->Get(i);
        // Need to notify that this render command does not have a render object.
        if (!renderCmd.target) continue;
        RenderObject* renderObj = renderCmd.target;
        if (!renderObj->Renderable) continue;
        Material* mat = renderObj->MaterialId;
        VkDescriptorSet descriptorSets[] = {
          mGlobalMat->Set()->Handle(),
          renderObj->CurrSet()->Handle(),
          mLightMat->Set()->Handle()
        };

        GraphicsPipeline* pipeline = renderObj->Skinned ? pbrPipeline : staticPbrPipeline;
        cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->Pipeline());
        cmdBuffer->SetViewPorts(0, 1, &viewport);

        // Bind materials.
        cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, 
          pipeline->Layout(), 
          0,
          3, 
          descriptorSets, 
          0, 
          nullptr
        );
  
        // Set up the render group.
        for (size_t idx = 0; idx < renderObj->Size(); ++idx) {
          MeshData* data = (*renderObj)[idx];

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
            cmdBuffer->DrawIndexed(indexBuffer->IndexCount(), renderObj->Instances, 0, 0, 0);
          } else {
            cmdBuffer->Draw(vertexBuffer->VertexCount(), renderObj->Instances, 0, 0);
          }
        }
      }
    }

    if (mDeferredCmdList) {
      for (size_t i = 0; i < mDeferredCmdList->Size(); ++i) {

      }
    }
  cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::BuildHDRCmdBuffer(u32 cmdBufferIndex)
{
  if (cmdBufferIndex >= mHDR.cmdBuffers.size()) {
    R_DEBUG(rError, "Attempted to build HDR cmd buffer. Index out of bounds!\n");
    return;
  }

  CommandBuffer* cmdBuffer = mHDR.cmdBuffers[cmdBufferIndex];
  if (!cmdBuffer) return;

  VkBuffer vertexBuffer = mRenderQuad.Quad()->Handle()->NativeBuffer();
  VkBuffer indexBuffer = mRenderQuad.Indices()->Handle()->NativeBuffer();
  VkDeviceSize offsets[] = { 0 };

  GraphicsPipeline* hdrPipeline = gResources().GetGraphicsPipeline(HDRGammaPipelineStr);
  GraphicsPipeline* Downscale2x = gResources().GetGraphicsPipeline(DownscaleBlurPipeline2xStr);
  GraphicsPipeline* Downscale4x = gResources().GetGraphicsPipeline(DownscaleBlurPipeline4xStr);
  GraphicsPipeline* Downscale8x = gResources().GetGraphicsPipeline(DownscaleBlurPipeline8xStr);
  GraphicsPipeline* GlowPipeline = gResources().GetGraphicsPipeline(GlowPipelineStr);
  FrameBuffer* hdrFrameBuffer = gResources().GetFrameBuffer(HDRGammaFrameBufferStr);
  FrameBuffer* DownscaleFrameBuffer2x = gResources().GetFrameBuffer(FrameBuffer2xHorizStr);
  FrameBuffer* FB2xFinal = gResources().GetFrameBuffer(FrameBuffer2xFinalStr);
  FrameBuffer* DownscaleFrameBuffer4x = gResources().GetFrameBuffer(FrameBuffer4xStr);
  FrameBuffer* FB4xFinal = gResources().GetFrameBuffer(FrameBuffer4xFinalStr);
  FrameBuffer* DownscaleFrameBuffer8x = gResources().GetFrameBuffer(FrameBuffer8xStr);
  FrameBuffer* FB8xFinal = gResources().GetFrameBuffer(FrameBuffer8xFinalStr);
  FrameBuffer* GlowFrameBuffer = gResources().GetFrameBuffer(FrameBufferGlowStr);
  DescriptorSet* hdrSet = gResources().GetDescriptorSet(HDRGammaDescSetStr);
  DescriptorSet* DownscaleSet2x = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet2x);
  DescriptorSet* DownscaleSet4x = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet4x);
  DescriptorSet* DownscaleSet8x = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet8x);
  DescriptorSet* DownscaleSet2xFinal = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet2xFinalStr);
  DescriptorSet* DownscaleSet4xFinal = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet4xFinalStr);
  DescriptorSet* DownscaleSet8xFinal = gResources().GetDescriptorSet(DownscaleBlurDescriptorSet8xFinalStr);
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
  renderpassInfo.renderArea.extent = mRhi->SwapchainObject()->SwapchainExtent();
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

  VkRenderPassBeginInfo GlowPass = {};
  GlowPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  GlowPass.framebuffer = GlowFrameBuffer->Handle();
  GlowPass.renderPass = GlowFrameBuffer->RenderPass();
  GlowPass.clearValueCount = 1;
  GlowPass.pClearValues = &clearVal;
  GlowPass.renderArea.extent = { GlowFrameBuffer->Width(), GlowFrameBuffer->Height() };
  GlowPass.renderArea.offset = { 0, 0 };

  VkViewport viewport = {};
  viewport.height = (r32)mWindowHandle->Height();
  viewport.width = (r32)mWindowHandle->Width();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.y = 0.0f;
  viewport.x = 0.0f;

  cmdBuffer->Begin(cmdBi);
    // TODO(): Need to allow switching on/off bloom passing.
    m_Downscale.strength = 0.45f;
    m_Downscale.scale = 4.0f;
    m_Downscale.horizontal = true;
    VkDescriptorSet DownscaleSetNative = DownscaleSet2x->Handle();
    viewport.height = (r32)(mWindowHandle->Height() >> 1);
    viewport.width =  (r32)(mWindowHandle->Width() >> 1);
    cmdBuffer->BeginRenderPass(DownscalePass2x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(r32), &m_Downscale.horizontal);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 4, sizeof(r32), &m_Downscale.strength);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 8, sizeof(r32), &m_Downscale.scale);
      cmdBuffer->DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();
    DownscalePass2x.framebuffer = FB2xFinal->Handle();
    DownscalePass2x.renderPass = FB2xFinal->RenderPass();
    cmdBuffer->BeginRenderPass(DownscalePass2x, VK_SUBPASS_CONTENTS_INLINE);
      m_Downscale.horizontal = false;
      DownscaleSetNative = DownscaleSet2xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale2x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale2x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &m_Downscale.horizontal);
      cmdBuffer->DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)(mWindowHandle->Height() >> 2);
    viewport.width = (r32)(mWindowHandle->Width() >> 2);
    DownscaleSetNative = DownscaleSet4x->Handle();
    i32 horizontal = true;
    cmdBuffer->BeginRenderPass(DownscalePass4x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Pipeline());
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &horizontal);
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    DownscalePass4x.framebuffer = FB4xFinal->Handle();
    DownscalePass4x.renderPass = FB4xFinal->RenderPass();
    cmdBuffer->BeginRenderPass(DownscalePass4x, VK_SUBPASS_CONTENTS_INLINE);
      horizontal = false;
      DownscaleSetNative = DownscaleSet4xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale4x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &horizontal);
      cmdBuffer->DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)(mWindowHandle->Height() >> 3);
    viewport.width = (r32)(mWindowHandle->Width() >> 3);
    DownscaleSetNative = DownscaleSet8x->Handle();
    horizontal = true;
    cmdBuffer->BeginRenderPass(DownscalePass8x, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Pipeline());
      cmdBuffer->PushConstants(Downscale8x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &horizontal);
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    DownscalePass8x.framebuffer = FB8xFinal->Handle();
    DownscalePass8x.renderPass = FB8xFinal->RenderPass();
    cmdBuffer->BeginRenderPass(DownscalePass8x, VK_SUBPASS_CONTENTS_INLINE);
      horizontal = false;
      DownscaleSetNative = DownscaleSet8xFinal->Handle();
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, Downscale8x->Layout(), 0, 1, &DownscaleSetNative, 0, nullptr);
      cmdBuffer->PushConstants(Downscale4x->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(i32), &horizontal);
      cmdBuffer->DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    viewport.height = (r32)mWindowHandle->Height();
    viewport.width = (r32)mWindowHandle->Width();
    VkDescriptorSet GlowDescriptorNative = GlowSet->Handle();
    cmdBuffer->BeginRenderPass(GlowPass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GlowPipeline->Layout(), 0, 1, &GlowDescriptorNative, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();

    VkDescriptorSet dSet = hdrSet->Handle();
    cmdBuffer->BeginRenderPass(renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Pipeline());
      cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Layout(), 0, 1, &dSet, 0, nullptr);
      cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
      cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer->DrawIndexed(mRenderQuad.Indices()->IndexCount(), 1, 0, 0, 0);
      cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::SetUpFinalOutputs()
{
  DescriptorSetLayout* finalSetLayout = gResources().GetDescriptorSetLayout(FinalDescSetLayoutStr);
  DescriptorSet* offscreenImageDescriptor = mRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet(FinalDescSetStr, offscreenImageDescriptor);
  offscreenImageDescriptor->Allocate(mRhi->DescriptorPool(), finalSetLayout);

  Texture* pbrColor = gResources().GetRenderTexture(PBRColorAttachStr);
  Texture* hdrColor = gResources().GetRenderTexture(
  HDRGammaColorAttachStr
  );
  Sampler* hdrSampler = gResources().GetSampler(HDRGammaSamplerStr);
  Sampler* pbrSampler = gResources().GetSampler(PBRSamplerStr);

  // TODO(): Final texture must be the hdr post process texture instead!
  VkDescriptorImageInfo renderTextureFinal = {};
  renderTextureFinal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  if (mHDR.enabled) {
    renderTextureFinal.sampler = hdrSampler->Handle();
    renderTextureFinal.imageView = hdrColor->View();
  } else {
    renderTextureFinal.sampler = pbrSampler->Handle();
    renderTextureFinal.imageView = pbrColor->View();
  }

  VkWriteDescriptorSet writeInfo = {};
  writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo.descriptorCount = 1;
  writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo.dstBinding = 0;
  writeInfo.pImageInfo = &renderTextureFinal;
  writeInfo.pBufferInfo = nullptr;
  writeInfo.pTexelBufferView = nullptr;
  writeInfo.dstArrayElement = 0;

  offscreenImageDescriptor->Update(1, &writeInfo);
}


void Renderer::CleanUpFinalOutputs()
{
  DescriptorSet* offscreenDescriptorSet = gResources().UnregisterDescriptorSet(FinalDescSetStr);
  mRhi->FreeDescriptorSet(offscreenDescriptorSet);
}


void Renderer::SetExposure(r32 e)
{
  mHDR.data.exposure = e;
}


void Renderer::SetGamma(r32 g)
{
  mHDR.data.gamma = g;
}


MeshData* Renderer::CreateMeshData()
{
  MeshData* mesh = new MeshData();
  mesh->mRhi = mRhi;
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
  obj->mRhi = mRhi;
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
  Buffer* hdr = mHDR.hdrBuffer;
  hdr->Map();
    memcpy(hdr->Mapped(), &mHDR.data, sizeof(mHDR.data));
  hdr->UnMap();
}


void Renderer::RenderOverlay()
{
  mUI->Render();
}


void Renderer::UpdateRendererConfigs(UserParams* params)
{
  mRhi->DeviceWaitIdle();

  if (mWindowHandle->Width() <= 0 || mWindowHandle <= 0) return;
  VkPresentModeKHR presentMode = mRhi->SwapchainObject()->CurrentPresentMode();

  if (params) {
    switch (params->presentMode) {
      case SINGLE_BUFFERING: presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; break;
      case DOUBLE_BUFFERING: presentMode = VK_PRESENT_MODE_FIFO_KHR; break;
      case TRIPLE_BUFFERING: presentMode = VK_PRESENT_MODE_MAILBOX_KHR; break;
      default: presentMode = VK_PRESENT_MODE_FIFO_KHR; break;
    }
  }

  // Triple buffering atm, we will need to use user params to switch this.
  mRhi->ReConfigure(presentMode, mWindowHandle->Width(), mWindowHandle->Height());

  mUI->CleanUp();

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

  mUI->Initialize(mRhi);
  if (mCmdList->Size() > 0) {
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
    u32 idx = mRhi->CurrentImageIndex();


    inProgress = false;
  });
}


void Renderer::WaitIdle()
{
  mRhi->DeviceWaitIdle();
}


GlobalMaterial* Renderer::CreateGlobalMaterial()
{
  GlobalMaterial* gMat = new GlobalMaterial();
  gMat->mRhi = mRhi;
  return gMat;
}


void Renderer::FreeGlobalMaterial(GlobalMaterial* material)
{
  material->CleanUp();
  delete material;
}


LightMaterial* Renderer::CreateLightMaterial()
{
  LightMaterial* lMat = new LightMaterial();
  lMat->mRhi = mRhi;

  return lMat;
}


void Renderer::FreeLightMaterial(LightMaterial* material)
{
  material->CleanUp();
  delete material;
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
  texture->mRhi = mRhi;

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

void Renderer::EnableBloom(b8 enable)
{
  mHDR.data.bloomEnabled = enable;
}


void Renderer::EnableHDR(b8 enable)
{
  if (mHDR.enabled != enable) {
    mHDR.enabled = enable;
    UpdateRendererConfigs(nullptr);
  }
}
} // Recluse