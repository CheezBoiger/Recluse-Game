// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Renderer.hpp"
#include "CmdList.hpp"
#include "Vertex.hpp"
#include "ScreenQuad.hpp"
#include "Mesh.hpp"
#include "CmdList.hpp"
#include "RenderCmd.hpp"
#include "Material.hpp"

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
{
}


Renderer::~Renderer()
{
}

void Renderer::OnStartUp()
{
  if (!gCore().IsActive()) {
    R_DEBUG("ERROR: Core is not active! Start up the core first!\n");
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
  VkCommandBuffer offscreenCmd = mOffscreen.cmdBuffer->Handle();
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

  // Update materials before rendering the frame.
  UpdateMaterials();

  // begin frame. This is where we start our render process per frame.
  BeginFrame();
    while (mOffscreen.cmdBuffer->Recording()) { }

    mRhi->GraphicsSubmit(offscreenSI);

    // Before calling this cmd buffer, we want to submit our offscreen buffer first, then
    // ssent our signal to our swapchain cmd buffers.
    VkSemaphore waitSemaphores[] = { mOffscreen.semaphore->Handle() };
    mRhi->SubmitCurrSwapchainCmdBuffer(1, waitSemaphores);

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
  mUI.CleanUp();

  mScreenQuad.CleanUp();
  CleanUpOffscreen();
  CleanUpGraphicsPipelines();
  CleanUpFrameBuffers();
  CleanUpRenderTextures();

  if (mRhi) {
    mRhi->CleanUp();
    delete mRhi;
    mRhi = nullptr;
  }
}


b8 Renderer::Initialize(Window* window)
{
  if (!window) return false;
  
  mWindowHandle = window;
  mRhi->Initialize(window->Handle());

  SetUpRenderTextures();
  SetUpFrameBuffers();
  SetUpGraphicsPipelines();
  SetUpOffscreen();
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

    cmdBuffer.BeginRenderPass(defaultRenderpass, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer.SetViewPorts(0, 1, &viewport);
      cmdBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline->Pipeline());

      VkBuffer vertexBuffer = mScreenQuad.Quad()->Handle()->Handle();
      VkBuffer indexBuffer = mScreenQuad.Indices()->Handle()->Handle();
      VkDeviceSize offsets[] = { 0 };

      cmdBuffer.BindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      cmdBuffer.BindVertexBuffers(0, 1, &vertexBuffer, offsets);

      cmdBuffer.DrawIndexed(mScreenQuad.Indices()->IndexCount(), 1, 0, 0, 0);
    cmdBuffer.EndRenderPass();
  });

  mUI.Initialize(mRhi);
  return true;
}


void Renderer::SetUpFrameBuffers()
{
  Texture* pbrColor = gResources().GetRenderTexture("PBRColor");
  Texture* pbrDepth = gResources().GetRenderTexture("PBRDepth");

  FrameBuffer* pbrFrameBuffer = mRhi->CreateFrameBuffer();
  gResources().RegisterFrameBuffer("PBRFrameBuffer", pbrFrameBuffer);


  VkAttachmentDescription attachmentDescriptions[2];
  attachmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
  attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[0].flags = 0;
  
  attachmentDescriptions[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
  attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
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

  VkDescriptorSetLayoutBinding layoutBinding0 = { };
  layoutBinding0.binding = 0;
  layoutBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBinding0.descriptorCount = 1;
  layoutBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layout0 = { };
  layout0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;  
  layout0.bindingCount = 1;
  layout0.pBindings = &layoutBinding0;

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

  VkDescriptorSetLayoutCreateInfo layout1 = { };
  layout1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout1.bindingCount = 8;
  layout1.pBindings = bindings;
  
  DescriptorSetLayout* d1 = mRhi->CreateDescriptorSetLayout();
  d1->Initialize(layout1);

  VkDescriptorSetLayoutBinding bind2 = { };
  bind2.binding = 0;
  bind2.descriptorCount = 1;
  bind2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bind2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layout2 = { };
  layout2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout2.bindingCount = 1;
  layout2.pBindings = &bind2;

  DescriptorSetLayout* d2 = mRhi->CreateDescriptorSetLayout();
  d2->Initialize(layout2);

  gResources().RegisterDescriptorSetLayout("PBRGlobalMaterialLayout", d0);
  gResources().RegisterDescriptorSetLayout("PBRObjectMaterialLayout", d1);  
  gResources().RegisterDescriptorSetLayout("PBRLightMaterialLayout",  d2);


  VkDescriptorSetLayout dLayouts[3];
  dLayouts[0] = d0->Layout();
  dLayouts[1] = d1->Layout();
  dLayouts[2] = d2->Layout();

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

  DescriptorSetLayout* finalSetLayout = mRhi->CreateDescriptorSetLayout();
  gResources().RegisterDescriptorSetLayout("FinalSetLayout", finalSetLayout);
  
  VkDescriptorSetLayoutBinding finalTextureSample = { };
  finalTextureSample.binding = 0;
  finalTextureSample.descriptorCount = 1;
  finalTextureSample.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  finalTextureSample.pImmutableSamplers = nullptr;
  finalTextureSample.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; 

  VkDescriptorSetLayoutCreateInfo finalLayoutInfo = { };
  finalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  finalLayoutInfo.bindingCount = 1;
  finalLayoutInfo.pBindings = &finalTextureSample;

  finalSetLayout->Initialize(finalLayoutInfo);

  VkPipelineLayoutCreateInfo finalLayout = {};
  finalLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  finalLayout.setLayoutCount = 1;
  VkDescriptorSetLayout finalL = finalSetLayout->Layout();
  finalLayout.pSetLayouts = &finalL;

  quadPipeline->Initialize(graphicsPipeline, finalLayout);

  mRhi->FreeShader(quadVert);
  mRhi->FreeShader(quadFrag);
}


void Renderer::CleanUpGraphicsPipelines()
{

  DescriptorSetLayout* d0 = gResources().UnregisterDescriptorSetLayout("PBRGlobalMaterialLayout");
  DescriptorSetLayout* d1 = gResources().UnregisterDescriptorSetLayout("PBRObjectMaterialLayout");
  DescriptorSetLayout* d2 = gResources().UnregisterDescriptorSetLayout("PBRLightMaterialLayout");

  mRhi->FreeDescriptorSetLayout(d0);
  mRhi->FreeDescriptorSetLayout(d1);
  mRhi->FreeDescriptorSetLayout(d2);

  GraphicsPipeline* pbrPipeline = gResources().UnregisterGraphicsPipeline("PBRPipeline");
  mRhi->FreeGraphicsPipeline(pbrPipeline);

  DescriptorSetLayout* finalSetLayout = gResources().UnregisterDescriptorSetLayout("FinalSetLayout");
  mRhi->FreeDescriptorSetLayout(finalSetLayout);

  GraphicsPipeline* quadPipeline = gResources().UnregisterGraphicsPipeline("FinalPassPipeline");
  mRhi->FreeGraphicsPipeline(quadPipeline);

}


void Renderer::CleanUpFrameBuffers()
{
  FrameBuffer* pbrFrameBuffer = gResources().UnregisterFrameBuffer("PBRFrameBuffer");
  mRhi->FreeFrameBuffer(pbrFrameBuffer);
}


void Renderer::SetUpRenderTextures()
{
  Texture* pbrColor = mRhi->CreateTexture();
  Texture* pbrDepth = mRhi->CreateTexture();
  Sampler* pbrSampler = mRhi->CreateSampler();

  gResources().RegisterRenderTexture("PBRColor", pbrColor);
  gResources().RegisterRenderTexture("PBRDepth", pbrDepth);
  gResources().RegisterSampler("PBRSampler", pbrSampler);
  
  VkImageCreateInfo cImageInfo = { };
  VkImageViewCreateInfo cViewInfo = { };

  cImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  cImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  cImageInfo.imageType = VK_IMAGE_TYPE_2D;
  cImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
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
  cViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  cViewInfo.image = nullptr; // No need to set the image, texture handles this for us.
  cViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  cViewInfo.subresourceRange = { };
  cViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  cViewInfo.subresourceRange.baseArrayLayer = 0;
  cViewInfo.subresourceRange.baseMipLevel = 0;
  cViewInfo.subresourceRange.layerCount = 1;
  cViewInfo.subresourceRange.levelCount = 1;

  pbrColor->Initialize(cImageInfo, cViewInfo);

  cImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  cImageInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;

  cViewInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
  cViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

  pbrDepth->Initialize(cImageInfo, cViewInfo);
}


void Renderer::CleanUpRenderTextures()
{
  Texture* pbrColor = gResources().UnregisterRenderTexture("PBRColor");
  Texture* pbrDepth = gResources().UnregisterRenderTexture("PBRDepth");
  Sampler* pbrSampler = gResources().UnregisterSampler("PBRSampler");

  mRhi->FreeTexture(pbrColor);
  mRhi->FreeTexture(pbrDepth);
  mRhi->FreeSampler(pbrSampler);
}


void Renderer::SetUpOffscreen()
{
  mOffscreen.semaphore = mRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCI = { };
  semaCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  mOffscreen.semaphore->Initialize(semaCI);

  mOffscreen.cmdBuffer = mRhi->CreateCommandBuffer();
  mOffscreen.cmdBuffer->Allocate(mRhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}


void Renderer::CleanUpOffscreen()
{
  mRhi->FreeVkSemaphore(mOffscreen.semaphore);
  mRhi->FreeCommandBuffer(mOffscreen.cmdBuffer);
}


void Renderer::Build()
{
  FrameBuffer* pbrBuffer = gResources().GetFrameBuffer("PBRFrameBuffer");
  GraphicsPipeline* pbrPipeline = gResources().GetGraphicsPipeline("PBRPipeline");

  // TODO(): Build offscreen cmd buffer, then call this function.
  if (mOffscreen.cmdBuffer && !mOffscreen.cmdBuffer->Recording()) {
    mRhi->DeviceWaitIdle();
    mOffscreen.cmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    MaterialCache.resize(0);
  }

  CommandBuffer* cmdBuffer = mOffscreen.cmdBuffer;

  VkCommandBufferBeginInfo beginInfo = { };
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VkClearValue clearValues[2];
  clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
  clearValues[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo pbrRenderPassInfo = { };
  pbrRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  pbrRenderPassInfo.framebuffer = pbrBuffer->Handle();
  pbrRenderPassInfo.renderPass = pbrBuffer->RenderPass();
  pbrRenderPassInfo.pClearValues = clearValues;
  pbrRenderPassInfo.clearValueCount = 2;
  pbrRenderPassInfo.renderArea.extent = mRhi->SwapchainObject()->SwapchainExtent();
  pbrRenderPassInfo.renderArea.offset = { 0, 0 };

  VkViewport viewport = {};
  viewport.height = (r32)mWindowHandle->Height();
  viewport.width = (r32)mWindowHandle->Width();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  viewport.x = 0.0f;
  viewport.y = 0.0f;

  cmdBuffer->Begin(beginInfo);
    cmdBuffer->BeginRenderPass(pbrRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      cmdBuffer->SetViewPorts(0, 1, &viewport);
      cmdBuffer->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline->Pipeline());
      if (mCmdList) {
        for (size_t i = 0; i < mCmdList->Size(); ++i) {
          RenderCmd* renderCmd = mCmdList->Get(i);

          // Extract material info. This is optional.
          if (renderCmd->materialId) {
            MaterialCache.push_back(renderCmd->materialId);

            if (renderCmd->meshId) {
              Material* mat = renderCmd->materialId;
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
          if (renderCmd->meshId && renderCmd->meshId->Renderable() && renderCmd->meshId->Visible()) {
            VertexBuffer* vertexBuffer = renderCmd->meshId->GetVertexBuffer();
            IndexBuffer* indexBuffer = renderCmd->meshId->GetIndexBuffer();
            VkBuffer vb = vertexBuffer->Handle()->Handle();
            VkBuffer ib = indexBuffer->Handle()->Handle();

            cmdBuffer->BindVertexBuffers(0, 1, &vb, 0);
            cmdBuffer->BindIndexBuffer(ib, 0, VK_INDEX_TYPE_UINT16);
          }
        }
      }

      if (mDeferredCmdList) {
        for (size_t i = 0; i < mDeferredCmdList->Size(); ++i) {
          
        }
      }
    cmdBuffer->EndRenderPass();
  cmdBuffer->End();

  mRhi->RebuildCommandBuffers();
}


Mesh* Renderer::CreateMesh()
{
  return nullptr;
}


void Renderer::UpdateMaterials()
{
  for (size_t i = 0; i < MaterialCache.size(); ++i) {
    Material* mat = MaterialCache[i];
    mat->Update();

    VkWriteDescriptorSet writeInfo = { };
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    
     // TODO(): We will hold off on this until we implement material.
    // We are going to need to store uniform buffers and samplers to our
    // material.
    //mat->ObjectBufferSet()->Update(writeInfo);
  }
}


void Renderer::RenderOverlay()
{
  mUI.Render();
}


void Renderer::UIOverlay::Render()
{
  // Ignore if no reference to the rhi.
  if (!mRhiRef) return;

  // Render the overlay.
}


void Renderer::UIOverlay::Initialize(VulkanRHI* rhi)
{
  mRhiRef = rhi;

  
}


void Renderer::UIOverlay::CleanUp()
{
  
}
} // Recluse