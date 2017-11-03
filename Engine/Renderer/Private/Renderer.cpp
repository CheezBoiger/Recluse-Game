// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Renderer.hpp"
#include "CmdList.hpp"
#include "Vertex.hpp"
#include "ScreenQuad.hpp"
#include "Mesh.hpp"
#include "CmdList.hpp"
#include "RenderCmd.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "UserParams.hpp"
#include "TextureType.hpp"
#include "UIOverlay.hpp"

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


std::vector<Material*> MaterialCache;


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
}


Renderer::~Renderer()
{
}

void Renderer::OnStartUp()
{
  if (!gCore().IsActive()) {
    R_DEBUG(rError, "Core is not active! Start up the core first!");
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

  mScreenQuad.CleanUp();
  CleanUpHDR(true);
  CleanUpOffscreen(true);
  CleanUpFinalOutputs();
  CleanUpDescriptorSetLayouts();
  CleanUpGraphicsPipelines();
  CleanUpFrameBuffers();
  CleanUpRenderTextures();

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

  SetUpRenderTextures();
  SetUpFrameBuffers();
  SetUpDescriptorSetLayouts();
  SetUpGraphicsPipelines();
  SetUpFinalOutputs();
  SetUpOffscreen(true);
  SetUpHDR(true);
  mScreenQuad.Initialize(mRhi);

  mRhi->SetSwapchainCmdBufferBuild([&] (CommandBuffer& cmdBuffer, VkRenderPassBeginInfo& defaultRenderpass) -> void {
    // Do stuff with the buffer.
    VkViewport viewport = { };
    viewport.height = (r32) mWindowHandle->Height();
    viewport.width = (r32) mWindowHandle->Width();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    
    GraphicsPipeline* finalPipeline = gResources().GetGraphicsPipeline("FinalPassPipeline");
    DescriptorSet* finalRenderTexture = gResources().GetDescriptorSet("OffscreenDescriptorSet");
    
    cmdBuffer.BeginRenderPass(defaultRenderpass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer.SetViewPorts(0, 1, &viewport);
      cmdBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Pipeline());
      VkDescriptorSet finalDescriptorSet = finalRenderTexture->Handle();    

      cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Layout(), 0, 1, &finalDescriptorSet, 0, nullptr);
      VkBuffer vertexBuffer = mScreenQuad.Quad()->Handle()->Handle();
      VkBuffer indexBuffer = mScreenQuad.Indices()->Handle()->Handle();
      VkDeviceSize offsets[] = { 0 };

      cmdBuffer.BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer.BindVertexBuffers(0, 1, &vertexBuffer, offsets);

      cmdBuffer.DrawIndexed(mScreenQuad.Indices()->IndexCount(), 1, 0, 0, 0);
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

  VkDescriptorSetLayoutBinding bindings[8];
  bindings[0].binding = 0;
  bindings[0].descriptorCount = 1;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].pImmutableSamplers = nullptr;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorCount = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[1].pImmutableSamplers = nullptr;
  bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  // albedo
  bindings[2].binding = 2;
  bindings[2].descriptorCount = 1;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[2].pImmutableSamplers = nullptr;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // metallic
  bindings[3].binding = 3;
  bindings[3].descriptorCount = 1;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[3].pImmutableSamplers = nullptr;
  bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // roughness
  bindings[4].binding = 4;
  bindings[4].descriptorCount = 1;
  bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[4].pImmutableSamplers = nullptr;
  bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // normal
  bindings[5].binding = 5;
  bindings[5].descriptorCount = 1;
  bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[5].pImmutableSamplers = nullptr;
  bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // ao
  bindings[6].binding = 6;
  bindings[6].descriptorCount = 1;
  bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[6].pImmutableSamplers = nullptr;
  bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // emissive
  bindings[7].binding = 7;
  bindings[7].descriptorCount = 1;
  bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[7].pImmutableSamplers = nullptr;
  bindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layout1 = {};
  layout1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout1.bindingCount = 8;
  layout1.pBindings = bindings;

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

  gResources().RegisterDescriptorSetLayout("PBRGlobalMaterialLayout", d0);
  gResources().RegisterDescriptorSetLayout("PBRObjectMaterialLayout", d1);
  gResources().RegisterDescriptorSetLayout("PBRLightMaterialLayout", d2);

  DescriptorSetLayout* finalSetLayout = mRhi->CreateDescriptorSetLayout();
  gResources().RegisterDescriptorSetLayout("FinalSetLayout", finalSetLayout);

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
  gResources().RegisterDescriptorSetLayout("HDRGammaLayout", hdrSetLayout);
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
}


void Renderer::CleanUpDescriptorSetLayouts()
{
  DescriptorSetLayout* d0 = gResources().UnregisterDescriptorSetLayout("PBRGlobalMaterialLayout");
  DescriptorSetLayout* d1 = gResources().UnregisterDescriptorSetLayout("PBRObjectMaterialLayout");
  DescriptorSetLayout* d2 = gResources().UnregisterDescriptorSetLayout("PBRLightMaterialLayout");

  mRhi->FreeDescriptorSetLayout(d0);
  mRhi->FreeDescriptorSetLayout(d1);
  mRhi->FreeDescriptorSetLayout(d2);

  DescriptorSetLayout* finalSetLayout = gResources().UnregisterDescriptorSetLayout("FinalSetLayout");
  mRhi->FreeDescriptorSetLayout(finalSetLayout);

  DescriptorSetLayout* hdrSetLayout = gResources().UnregisterDescriptorSetLayout("HDRGammaLayout");
  mRhi->FreeDescriptorSetLayout(hdrSetLayout);
}


void Renderer::SetUpFrameBuffers()
{
  Texture* pbrColor = gResources().GetRenderTexture("PBRColor");
  Texture* pbrDepth = gResources().GetRenderTexture("PBRDepth");

  FrameBuffer* pbrFrameBuffer = mRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer("PBRFrameBuffer", pbrFrameBuffer);

  FrameBuffer* hdrFrameBuffer = mRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer("HDRGammaFrameBuffer", hdrFrameBuffer);


  VkAttachmentDescription attachmentDescriptions[2];
  attachmentDescriptions[0].format = pbrColor->Format();
  attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
  attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[0].samples = pbrColor->Samples();
  attachmentDescriptions[0].flags = 0;
  
  attachmentDescriptions[1].format = pbrDepth->Format();
  attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[1].samples = pbrDepth->Samples();
  attachmentDescriptions[1].flags = 0;   

  VkSubpassDependency dependencies[2];
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;  

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkAttachmentReference attachmentColorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
  VkAttachmentReference attachmentDepthRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpass = { };
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &attachmentColorRef;
  subpass.pDepthStencilAttachment = &attachmentDepthRef;
  
  VkRenderPassCreateInfo renderpassCI = { };
  renderpassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpassCI.attachmentCount = 2;
  renderpassCI.pAttachments = attachmentDescriptions;
  renderpassCI.subpassCount = 1;
  renderpassCI.pSubpasses = &subpass;
  renderpassCI.dependencyCount = 2;
  renderpassCI.pDependencies = dependencies;


  VkImageView attachments[2];
  attachments[0] = pbrColor->View();
  attachments[1] = pbrDepth->View();

  VkFramebufferCreateInfo framebufferCI = {};
  framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferCI.height = mWindowHandle->Height();
  framebufferCI.width = mWindowHandle->Width();
  framebufferCI.renderPass = nullptr; // The finalize call handles this for us.
  framebufferCI.layers = 1;
  framebufferCI.attachmentCount = 2;
  framebufferCI.pAttachments = attachments;

  pbrFrameBuffer->Finalize(framebufferCI, renderpassCI);
  
  // No need to render any depth, as we are only writing on a 2d surface.
  Texture* hdrColor = gResources().GetRenderTexture("HDRGammaTexture");
  subpass.pDepthStencilAttachment = nullptr;
  attachments[0] = hdrColor->View();
  attachments[1] = nullptr;
  framebufferCI.attachmentCount = 1;
  attachmentDescriptions[0].format = hdrColor->Format();
  attachmentDescriptions[0].samples = hdrColor->Samples();
  renderpassCI.attachmentCount = 1;

  hdrFrameBuffer->Finalize(framebufferCI, renderpassCI);
}


void Renderer::SetUpGraphicsPipelines()
{
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

  VkPipelineRasterizationStateCreateInfo rasterizerCI = { };
  rasterizerCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerCI.depthClampEnable = VK_FALSE;
  rasterizerCI.rasterizerDiscardEnable = VK_FALSE;
  rasterizerCI.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizerCI.lineWidth = 1.0f;
  rasterizerCI.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizerCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizerCI.depthBiasEnable = VK_FALSE;
  rasterizerCI.depthBiasConstantFactor = 0.0f;
  rasterizerCI.depthBiasSlopeFactor = 0.0f;
  rasterizerCI.depthBiasClamp = 0.0f;

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

  VkPipelineColorBlendAttachmentState colorBlendAttachment = { };
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendCI = { };
  colorBlendCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; 
  colorBlendCI.attachmentCount = 1;
  colorBlendCI.pAttachments = &colorBlendAttachment;
  colorBlendCI.logicOpEnable = VK_FALSE;
  colorBlendCI.logicOp = VK_LOGIC_OP_COPY;
  colorBlendCI.blendConstants[0] = 0.0f;
  colorBlendCI.blendConstants[1] = 0.0f;
  colorBlendCI.blendConstants[2] = 0.0f;
  colorBlendCI.blendConstants[3] = 0.0f;

  VkDynamicState dynamicStates[1] = {
    VK_DYNAMIC_STATE_VIEWPORT
  };  

  VkPipelineDynamicStateCreateInfo dynamicCI = { };
  dynamicCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicCI.dynamicStateCount = 1;
  dynamicCI.pDynamicStates = dynamicStates;
  
  VkVertexInputBindingDescription vertBindingDesc = { };
  vertBindingDesc.binding = 0;
  vertBindingDesc.stride = sizeof(SkinnedVertex);
  vertBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  u32 offset = 0;
  VkVertexInputAttributeDescription pbrAttributes[7];
  pbrAttributes[0].binding = 0;
  pbrAttributes[0].location = 0;
  pbrAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  pbrAttributes[0].offset = offset;
  offset += sizeof(r32) * 4;

  pbrAttributes[1].binding = 0;
  pbrAttributes[1].location = 1;
  pbrAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  pbrAttributes[1].offset = offset;
  offset += sizeof(r32) * 4;

  pbrAttributes[2].binding = 0;
  pbrAttributes[2].location = 2;
  pbrAttributes[2].format = VK_FORMAT_R32G32_SFLOAT;
  pbrAttributes[2].offset = offset;
  offset += sizeof(r32) * 2;
  
  pbrAttributes[3].binding = 0;
  pbrAttributes[3].location = 3;
  pbrAttributes[3].format = VK_FORMAT_R32G32_SFLOAT;
  pbrAttributes[3].offset = offset;
  offset += sizeof(r32) * 2;

  pbrAttributes[4].binding = 0;
  pbrAttributes[4].location = 4;
  pbrAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  pbrAttributes[4].offset = offset;
  offset += sizeof(r32) * 4;
  
  pbrAttributes[5].binding = 0;
  pbrAttributes[5].location = 5;
  pbrAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;    
  pbrAttributes[5].offset = offset;
  offset += sizeof(r32) * 4;

  pbrAttributes[6].binding = 0;
  pbrAttributes[6].location = 6;
  pbrAttributes[6].format = VK_FORMAT_R32G32B32A32_SINT;
  pbrAttributes[6].offset = offset;
  offset += sizeof(i32) * 4;

  VkPipelineVertexInputStateCreateInfo vertexCI = { };
  vertexCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexCI.vertexBindingDescriptionCount = 1;
  vertexCI.pVertexBindingDescriptions = &vertBindingDesc;
  vertexCI.vertexAttributeDescriptionCount = 7;
  vertexCI.pVertexAttributeDescriptions = pbrAttributes;

  // PbrForward Pipeline Creation.
  GraphicsPipeline* pbrForwardPipeline = mRhi->CreateGraphicsPipeline();
  FrameBuffer* pbrFrameBuffer = gResources().GetFrameBuffer("PBRFrameBuffer");

  VkGraphicsPipelineCreateInfo graphicsPipeline = {};
  graphicsPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphicsPipeline.renderPass = pbrFrameBuffer->RenderPass();
  graphicsPipeline.pColorBlendState = &colorBlendCI;
  graphicsPipeline.pDepthStencilState = &depthStencilCI;
  graphicsPipeline.pInputAssemblyState = &assemblyCI;
  graphicsPipeline.pRasterizationState = &rasterizerCI;
  graphicsPipeline.pMultisampleState = &msCI;
  graphicsPipeline.pVertexInputState = &vertexCI;
  graphicsPipeline.pViewportState = &viewportCI;
  graphicsPipeline.pTessellationState = nullptr;
  graphicsPipeline.pDynamicState = &dynamicCI;
  graphicsPipeline.subpass = 0;

  Shader* mVertPBR = mRhi->CreateShader();
  Shader* mFragPBR = mRhi->CreateShader();

  std::string filepath = gFilesystem().CurrentAppDirectory();
  mVertPBR->Initialize(filepath + "/Shaders/PBRPass.vert.spv");
  mFragPBR->Initialize(filepath + "/Shaders/PBRPass.frag.spv");

  VkPipelineShaderStageCreateInfo pbrShaders[2];
  pbrShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pbrShaders[0].module = mVertPBR->Handle();
  pbrShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  pbrShaders[0].pName = "main";
  pbrShaders[0].pNext = nullptr;
  pbrShaders[0].pSpecializationInfo = nullptr;
  pbrShaders[0].flags = 0;

  pbrShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pbrShaders[1].module = mFragPBR->Handle();
  pbrShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  pbrShaders[1].pName = "main";
  pbrShaders[1].pNext = nullptr;
  pbrShaders[1].flags = 0;
  pbrShaders[1].pSpecializationInfo = nullptr;

  graphicsPipeline.stageCount = 2;
  graphicsPipeline.pStages = pbrShaders;

  graphicsPipeline.basePipelineHandle = VK_NULL_HANDLE;


  VkDescriptorSetLayout dLayouts[3];
  dLayouts[0] = gResources().GetDescriptorSetLayout("PBRGlobalMaterialLayout")->Layout();
  dLayouts[1] = gResources().GetDescriptorSetLayout("PBRObjectMaterialLayout")->Layout();
  dLayouts[2] = gResources().GetDescriptorSetLayout("PBRLightMaterialLayout")->Layout();

  VkPipelineLayoutCreateInfo pipelineLayout = { };
  pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayout.setLayoutCount = 3;
  pipelineLayout.pSetLayouts = dLayouts;
  pipelineLayout.pPushConstantRanges = 0;
  pipelineLayout.pushConstantRangeCount = 0;
  
  // Initialize pbr forward pipeline.
  pbrForwardPipeline->Initialize(graphicsPipeline, pipelineLayout);
  
  gResources().RegisterGraphicsPipeline("PBRPipeline", pbrForwardPipeline);

  mRhi->FreeShader(mVertPBR);
  mRhi->FreeShader(mFragPBR);  

  // TODO(): Need to structure all of this into more manageable modules.
  //
  GraphicsPipeline* quadPipeline = mRhi->CreateGraphicsPipeline();
  gResources().RegisterGraphicsPipeline("FinalPassPipeline", quadPipeline);

  // Set to default renderpass.
  graphicsPipeline.renderPass = mRhi->SwapchainRenderPass();
  depthStencilCI.depthTestEnable = VK_FALSE;
  depthStencilCI.stencilTestEnable = VK_FALSE;
  rasterizerCI.cullMode = VK_CULL_MODE_NONE;

  Shader* quadVert = mRhi->CreateShader();
  Shader* quadFrag = mRhi->CreateShader();

  quadVert->Initialize(filepath + "/Shaders/FinalPass.vert.spv");
  quadFrag->Initialize(filepath + "/Shaders/FinalPass.frag.spv");

  VkPipelineShaderStageCreateInfo finalShaders[2];
  finalShaders[0].flags = 0;
  finalShaders[0].module = quadVert->Handle();
  finalShaders[0].pName = "main";
  finalShaders[0].pNext = nullptr;
  finalShaders[0].pSpecializationInfo = nullptr;
  finalShaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  finalShaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  finalShaders[1].flags = 0;
  finalShaders[1].module = quadFrag->Handle();
  finalShaders[1].pName = "main";
  finalShaders[1].pNext = nullptr;
  finalShaders[1].pSpecializationInfo = nullptr;
  finalShaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  finalShaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  graphicsPipeline.stageCount = 2;
  graphicsPipeline.pStages = finalShaders;

  VkVertexInputAttributeDescription finalAttribs[2];
  finalAttribs[0].binding = 0;
  finalAttribs[0].format = VK_FORMAT_R32G32_SFLOAT;
  finalAttribs[0].location = 0;
  finalAttribs[0].offset = 0;
  
  finalAttribs[1].binding = 0;
  finalAttribs[1].format = VK_FORMAT_R32G32_SFLOAT;
  finalAttribs[1].location = 1;
  finalAttribs[1].offset = sizeof(r32) * 2;

  vertBindingDesc.binding = 0;
  vertBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertBindingDesc.stride = sizeof(QuadVertex);

  vertexCI.vertexAttributeDescriptionCount = 2;
  vertexCI.pVertexAttributeDescriptions = finalAttribs;

  VkPipelineLayoutCreateInfo finalLayout = {};
  finalLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  finalLayout.setLayoutCount = 1;
  VkDescriptorSetLayout finalL = gResources().GetDescriptorSetLayout("FinalSetLayout")->Layout();
  finalLayout.pSetLayouts = &finalL;

  quadPipeline->Initialize(graphicsPipeline, finalLayout);

  mRhi->FreeShader(quadVert);
  mRhi->FreeShader(quadFrag);

  // HDR Pipeline initialization.
  GraphicsPipeline* hdrPipeline = mRhi->CreateGraphicsPipeline();
  VkPipelineLayoutCreateInfo hdrLayout = { };
  VkDescriptorSetLayout hdrSetLayout = gResources().GetDescriptorSetLayout("HDRGammaLayout")->Layout();

  Shader* hdrFrag = mRhi->CreateShader();
  Shader* hdrVert = mRhi->CreateShader();

  hdrFrag->Initialize(filepath + "/Shaders/HDRGammaPass.frag.spv");
  hdrVert->Initialize(filepath + "/Shaders/HDRGammaPass.vert.spv");

  FrameBuffer* hdrBuffer = gResources().GetFrameBuffer("HDRGammaFrameBuffer");
  graphicsPipeline.renderPass = hdrBuffer->RenderPass();

  finalShaders[0].module = hdrVert->Handle();
  finalShaders[1].module = hdrFrag->Handle();

  hdrLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  hdrLayout.setLayoutCount = 1;
  hdrLayout.pSetLayouts = &hdrSetLayout;

  hdrPipeline->Initialize(graphicsPipeline, hdrLayout);

  mRhi->FreeShader(hdrFrag);
  mRhi->FreeShader(hdrVert);
  gResources().RegisterGraphicsPipeline("HDRGammaPipeline", hdrPipeline);
}


void Renderer::CleanUpGraphicsPipelines()
{
  GraphicsPipeline* pbrPipeline = gResources().UnregisterGraphicsPipeline("PBRPipeline");
  mRhi->FreeGraphicsPipeline(pbrPipeline);

  GraphicsPipeline* quadPipeline = gResources().UnregisterGraphicsPipeline("FinalPassPipeline");
  mRhi->FreeGraphicsPipeline(quadPipeline);

  GraphicsPipeline* hdrPipeline = gResources().UnregisterGraphicsPipeline("HDRGammaPipeline");
  mRhi->FreeGraphicsPipeline(hdrPipeline);
}


void Renderer::CleanUpFrameBuffers()
{
  FrameBuffer* pbrFrameBuffer = gResources().UnregisterFrameBuffer("PBRFrameBuffer");
  mRhi->FreeFrameBuffer(pbrFrameBuffer);


  FrameBuffer* hdrFrameBuffer = gResources().UnregisterFrameBuffer("HDRGammaFrameBuffer");
  mRhi->FreeFrameBuffer(hdrFrameBuffer);
}


void Renderer::SetUpRenderTextures()
{
  Texture* pbrColor = mRhi->CreateTexture();
  Texture* pbrDepth = mRhi->CreateTexture();
  Sampler* pbrSampler = mRhi->CreateSampler();
  Texture* hdrTexture = mRhi->CreateTexture();
  Sampler* hdrSampler = mRhi->CreateSampler();

  gResources().RegisterSampler("HDRGammaSampler", hdrSampler);
  gResources().RegisterRenderTexture("HDRGammaTexture", hdrTexture);
  gResources().RegisterRenderTexture("PBRColor", pbrColor);
  gResources().RegisterRenderTexture("PBRDepth", pbrDepth);
  gResources().RegisterSampler("PBRSampler", pbrSampler);
  
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

  cImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  hdrTexture->Initialize(cImageInfo, cViewInfo);

  cImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  cImageInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;

  cViewInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
  cViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

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

  Sampler* defaultSampler = mRhi->CreateSampler();
  defaultSampler->Initialize(samplerCI);
  gResources().RegisterSampler("DefaultSampler", defaultSampler);

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
  gResources().RegisterRenderTexture("DefaultTexture", defaultTexture);
}


void Renderer::CleanUpRenderTextures()
{
  Texture* pbrColor = gResources().UnregisterRenderTexture("PBRColor");
  Texture* pbrDepth = gResources().UnregisterRenderTexture("PBRDepth");
  Sampler* pbrSampler = gResources().UnregisterSampler("PBRSampler");

  Texture* hdrTexture = gResources().UnregisterRenderTexture("HDRGammaTexture");
  Sampler* hdrSampler = gResources().UnregisterSampler("HDRGammaSampler");
  
  mRhi->FreeTexture(hdrTexture);
  mRhi->FreeSampler(hdrSampler);

  Texture* defaultTexture = gResources().UnregisterRenderTexture("DefaultTexture");
  Sampler* defaultSampler = gResources().UnregisterSampler("DefaultSampler");

  mRhi->FreeTexture(pbrColor);
  mRhi->FreeTexture(pbrDepth);
  mRhi->FreeSampler(pbrSampler);

  mRhi->FreeTexture(defaultTexture);
  mRhi->FreeSampler(defaultSampler);
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
  gResources().RegisterDescriptorSet("HDRGammaSet", hdrSet);
  VkWriteDescriptorSet hdrWrites[3];
  VkDescriptorBufferInfo hdrBufferInfo = {};
  hdrBufferInfo.offset = 0;
  hdrBufferInfo.range = sizeof(mHDR.data);
  hdrBufferInfo.buffer = mHDR.hdrBuffer->Handle();

  VkDescriptorImageInfo pbrImageInfo = { };
  pbrImageInfo.sampler = gResources().GetSampler("PBRSampler")->Handle();
  pbrImageInfo.imageView = gResources().GetRenderTexture("PBRColor")->View();
  pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // TODO(): We don't have our bloom pipeline and texture yet, we will sub it with this instead!
  VkDescriptorImageInfo bloomImageInfo = { };
  bloomImageInfo.sampler = gResources().GetSampler("PBRSampler")->Handle();
  bloomImageInfo.imageView = gResources().GetRenderTexture("PBRColor")->View();
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
  hdrSet->Allocate(mRhi->DescriptorPool(), gResources().GetDescriptorSetLayout("HDRGammaLayout"));
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

  DescriptorSet* hdrSet = gResources().UnregisterDescriptorSet("HDRGammaSet");
  mRhi->FreeDescriptorSet(hdrSet);
}


void Renderer::Build()
{
  mRhi->GraphicsWaitIdle();

  BuildOffScreenBuffer(mOffscreen.currCmdBufferIndex);
  BuildHDRCmdBuffer(mHDR.currCmdBufferIndex);
  mRhi->RebuildCurrentCommandBuffers();
}


void Renderer::BuildOffScreenBuffer(u32 cmdBufferIndex)
{
  if (cmdBufferIndex >= mOffscreen.cmdBuffers.size()) { 
    R_DEBUG(rError, "Attempted to build offscreen cmd buffer. Index out of bounds!");
    return; 
  }

  CommandBuffer* cmdBuffer = mOffscreen.cmdBuffers[cmdBufferIndex];
  FrameBuffer* pbrBuffer = gResources().GetFrameBuffer("PBRFrameBuffer");
  GraphicsPipeline* pbrPipeline = gResources().GetGraphicsPipeline("PBRPipeline");

  if (mGlobalMat) {
    mGlobalMat->CleanUp();
    mGlobalMat->Initialize();
  }

  if (cmdBuffer && !cmdBuffer->Recording()) {

    mRhi->DeviceWaitIdle();
    cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    // We are only worried about the buffers in the gpu, which need to be recreated
    // in order to update the window resolutions and whatnot. Since the pipeline is recreated,
    // our mats need to be recreated as well.
    MaterialCache.resize(0);
  }

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VkClearValue clearValues[2];
  clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
  clearValues[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo pbrRenderPassInfo = {};
  pbrRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  pbrRenderPassInfo.framebuffer = pbrBuffer->Handle();
  pbrRenderPassInfo.renderPass = pbrBuffer->RenderPass();
  pbrRenderPassInfo.pClearValues = clearValues;
  pbrRenderPassInfo.clearValueCount = 2;
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
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline->Pipeline());
    cmdBuffer->SetViewPorts(0, 1, &viewport);
    if (mCmdList) {
      for (size_t i = 0; i < mCmdList->Size(); ++i) {
        RenderCmd& renderCmd = mCmdList->Get(i);

        // Extract material info. This is optional.
        if (renderCmd.materialId) {
          MaterialCache.push_back(renderCmd.materialId);

          if (renderCmd.meshId) {
            Material* mat = renderCmd.materialId;
            // Screen must be updated.
            mat->CleanUp();
            mat->Initialize();

            VkDescriptorSet descriptorSets[] = {
              mGlobalMat->Set()->Handle(),
              mat->Set()->Handle(),
              mLightMat->Set()->Handle()
            };

            cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline->Layout(), 0,
              3, descriptorSets, 0, nullptr);
          }
        }

        // Extract Mesh info. This is optional. 
        if (renderCmd.meshId && renderCmd.meshId->Renderable() && renderCmd.meshId->Visible()) {
          VertexBuffer* vertexBuffer = renderCmd.meshId->GetVertexBuffer();
          IndexBuffer* indexBuffer = renderCmd.meshId->GetIndexBuffer();
          VkBuffer vb = vertexBuffer->Handle()->Handle();
          VkBuffer ib = indexBuffer->Handle()->Handle();
          VkDeviceSize offsets[] = { 0 };
          cmdBuffer->BindVertexBuffers(0, 1, &vb, offsets);
          cmdBuffer->BindIndexBuffer(ib, 0, VK_INDEX_TYPE_UINT32);
          cmdBuffer->DrawIndexed(indexBuffer->IndexCount(), 1, 0, 0, 0);
          //cmdBuffer->Draw(vertexBuffer->VertexCount(), 1, 0, 0);
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
    R_DEBUG(rError, "Attempted to build HDR cmd buffer. Index out of bounds!");
    return;
  }

  CommandBuffer* cmdBuffer = mHDR.cmdBuffers[cmdBufferIndex];
  if (!cmdBuffer) return;

  GraphicsPipeline* hdrPipeline = gResources().GetGraphicsPipeline("HDRGammaPipeline");
  FrameBuffer* hdrFrameBuffer = gResources().GetFrameBuffer("HDRGammaFrameBuffer");
  DescriptorSet* hdrSet = gResources().GetDescriptorSet("HDRGammaSet");
  

  cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  VkCommandBufferBeginInfo cmdBi = { };
  cmdBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  

  VkClearValue clearVal = { };
  clearVal.color = { 0.2f, 0.2f, 0.2f, 1.0f };

  cmdBuffer->Begin(cmdBi);
    VkRenderPassBeginInfo renderpassInfo = { };
    renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpassInfo.framebuffer = hdrFrameBuffer->Handle();
    renderpassInfo.clearValueCount = 1;
    renderpassInfo.pClearValues =  &clearVal;
    renderpassInfo.renderPass = hdrFrameBuffer->RenderPass(); 
    renderpassInfo.renderArea.extent = mRhi->SwapchainObject()->SwapchainExtent();
    renderpassInfo.renderArea.offset = { 0, 0 };

    cmdBuffer->BeginRenderPass(renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.height = (r32)mWindowHandle->Height();
    viewport.width = (r32)mWindowHandle->Width();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.y = 0.0f;
    viewport.x = 0.0f;

    cmdBuffer->SetViewPorts(0, 1, &viewport);
    cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Pipeline());
    VkDescriptorSet dSet = hdrSet->Handle();
    cmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipeline->Layout(), 0, 1, &dSet, 0, nullptr);
    VkBuffer vertexBuffer = mScreenQuad.Quad()->Handle()->Handle();
    VkBuffer indexBuffer = mScreenQuad.Indices()->Handle()->Handle();
    VkDeviceSize offsets[] = { 0 };

    cmdBuffer->BindVertexBuffers(0, 1, &vertexBuffer, offsets);
    cmdBuffer->BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    cmdBuffer->DrawIndexed(mScreenQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer->EndRenderPass();
  cmdBuffer->End();
}


void Renderer::SetUpFinalOutputs()
{
  DescriptorSetLayout* finalSetLayout = gResources().GetDescriptorSetLayout("FinalSetLayout");
  DescriptorSet* offscreenImageDescriptor = mRhi->CreateDescriptorSet();
  gResources().RegisterDescriptorSet("OffscreenDescriptorSet", offscreenImageDescriptor);
  offscreenImageDescriptor->Allocate(mRhi->DescriptorPool(), finalSetLayout);

  Texture* pbrColor = gResources().GetRenderTexture("PBRColor");
  Texture* hdrColor = gResources().GetRenderTexture("HDRGammaTexture");
  Sampler* hdrSampler = gResources().GetSampler("HDRGammaSampler");
  Sampler* pbrSampler = gResources().GetSampler("PBRSampler");

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
  DescriptorSet* offscreenDescriptorSet = gResources().UnregisterDescriptorSet("OffscreenDescriptorSet");
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


Mesh* Renderer::CreateMesh()
{
  Mesh* mesh = new Mesh();
  mesh->mRhi = mRhi;
  return mesh;
}


void Renderer::FreeMesh(Mesh* mesh)
{
  mesh->CleanUp();
  delete mesh;
}


void Renderer::FreeMaterial(Material* material)
{
  material->CleanUp();
  delete material;
}


void Renderer::UpdateMaterials()
{
  if (mGlobalMat) {
    mGlobalMat->Update();
  }

  if (mLightMat) {
    mLightMat->Update();
  }

  for (size_t i = 0; i < MaterialCache.size(); ++i) {
    Material* mat = MaterialCache[i];
    mat->Update();
  }

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
  CleanUpOffscreen(false);
  CleanUpFinalOutputs();
  CleanUpGraphicsPipelines();
  CleanUpFrameBuffers();
  CleanUpRenderTextures();

  SetUpRenderTextures();
  SetUpFrameBuffers();
  SetUpGraphicsPipelines();
  SetUpFinalOutputs();
  SetUpOffscreen(false);
  SetUpHDR(false);

  mUI->Initialize(mRhi);

  Build();
}


void Renderer::BuildAsync()
{
  static b8 inProgress = false;
  // TODO(): building the command buffers asyncronously requires us
  // to allocate temp commandbuffers, build them, and then swap them with
  // the previous commandbuffers.
  std::thread async([] () -> void {
    if (inProgress) { return; }

    inProgress = true;


    

    inProgress = false;
  });
}


void Renderer::WaitIdle()
{
  mRhi->DeviceWaitIdle();
}


Material* Renderer::CreateMaterial()
{
  Material* material = new Material();
  material->mRhi = mRhi;
  return material;
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


void Renderer::EnableHDR(b8 enable)
{
  if (mHDR.enabled != enable) {
    mHDR.enabled = enable;
    UpdateRendererConfigs(nullptr);
  }
}
} // Recluse