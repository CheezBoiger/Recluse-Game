// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>


#define NVIDIA_VENDOR_ID        0x10DE
#define INTEL_VENDOR_ID         0x1F96
#define AMD_VENDOR_ID           0x1022

#define INDEX_SIZE_TYPE_16_BIT      2
#define INDEX_SIZE_TYPE_32_BIT      4

#define INTEL_WORKGROUP_SIZE       16   // Optimal EU hardware thread workgroup size on Intel chips.
#define NVIDIA_WARP_SIZE           16  // Optimal CUDA SIMT workgroup size on Nvidia chips.
#define AMD_WAVEFRONT_SIZE         16   // Optimal AMD workgroup size on Radeon chips.

#if defined(_DEBUG)
#define RDEBUG_SET_VULKAN_NAME(obj, name) obj->SetName(name)
#else
#define RDEBUG_SET_VULKAN_NAME(obj, name)
#endif

typedef unsigned long graphics_uuid_t;

class VulkanHandle {
  static graphics_uuid_t uuid;
public:
  
  VulkanHandle()
    : mOwner(VK_NULL_HANDLE)
    , m_uuid(uuid++) 
#if defined (_DEBUG)
    , m_name("")
#endif
    { }

  void      SetOwner(VkDevice owner) { mOwner = owner; }
  VkDevice  Owner() { return mOwner; }
  graphics_uuid_t GetUUID() const { return m_uuid; }

  DEBUG_OP(
  void      SetName(const char* name) { m_name = name; }
  const char*      GetName() { return m_name; }
  )

protected:
  VkDevice  mOwner;
  graphics_uuid_t m_uuid;
  DEBUG_OP(
  const char* m_name;
  )
};


namespace Recluse {
namespace {

VkSubpassDependency CreateSubPassDependency(u32 srcSubpass, VkAccessFlags srcAccessMask,
  VkPipelineStageFlags srcStageMask, u32 dstSubpass, VkAccessFlags dstAccessMask,
  VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags)
{
  VkSubpassDependency d = { };
  d.srcSubpass = srcSubpass;
  d.srcAccessMask = srcAccessMask;
  d.srcStageMask = srcStageMask;
  d.dstSubpass = dstSubpass;
  d.dstAccessMask = dstAccessMask;
  d.dstStageMask = dstStageMask;
  d.dependencyFlags = dependencyFlags;
  return d;
}


VkAttachmentDescription CreateAttachmentDescription(VkFormat format, VkImageLayout initialLayout,
  VkImageLayout finalLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, 
  VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp,
  VkSampleCountFlagBits samples, VkAttachmentDescriptionFlags flags = 0)
{
  VkAttachmentDescription desc = { };
  desc.format = format;
  desc.initialLayout = initialLayout;
  desc.finalLayout = finalLayout;
  desc.flags = flags;
  desc.loadOp = loadOp;
  desc.samples = samples;
  desc.stencilLoadOp = stencilLoadOp;
  desc.stencilStoreOp = stencilStoreOp;
  desc.storeOp = storeOp;

  return desc;
}


VkRenderPassCreateInfo CreateRenderPassInfo(Recluse::u32 attachmentCount, 
  VkAttachmentDescription* pAttachments, Recluse::u32 dependencyCount, VkSubpassDependency* pDependencies,
  u32 subpassCount, VkSubpassDescription* pSubpasses, VkRenderPassCreateFlags flags = 0, 
  void* pNext = nullptr)
{
  VkRenderPassCreateInfo rp = { };
  rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rp.attachmentCount = attachmentCount;
  rp.dependencyCount = dependencyCount;
  rp.flags = flags;
  rp.pAttachments = pAttachments;
  rp.pDependencies = pDependencies;
  rp.pNext = pNext;
  rp.pSubpasses = pSubpasses;
  rp.subpassCount = subpassCount;
  return rp;
}


VkFramebufferCreateInfo CreateFrameBufferInfo(u32 width, u32 height,
  VkRenderPass renderPass, u32 attachmentCount, const VkImageView* pAttachments,
  u32 layers, VkFramebufferCreateFlags flags = 0)
{
  VkFramebufferCreateInfo fb = { };
  fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fb.width = width;
  fb.height = height;
  fb.renderPass = renderPass;
  fb.attachmentCount = attachmentCount;
  fb.pAttachments = pAttachments;
  fb.layers = layers;
  fb.flags = flags;
  return fb;
}


VkPipelineRasterizationStateCreateInfo CreateRasterInfo(VkPolygonMode polygonMode, 
  VkBool32 rasterizerDiscardEnable, VkCullModeFlags cullMode, VkFrontFace frontFace, 
  r32 lineWidth, VkBool32 depthBiasEnable, VkBool32 depthClampEnable, r32 depthBiasClamp = 0.0f,
  r32 depthBiasSlopeFactor = 0.0f, VkPipelineRasterizationStateCreateFlags flags = 0
  )
{
  VkPipelineRasterizationStateCreateInfo raster = { };
  raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  raster.depthClampEnable = depthClampEnable;
  raster.rasterizerDiscardEnable = rasterizerDiscardEnable;
  raster.polygonMode = polygonMode;
  raster.cullMode = cullMode;
  raster.lineWidth = lineWidth;
  raster.frontFace = frontFace;
  raster.depthBiasEnable = depthBiasEnable;
  raster.depthBiasSlopeFactor = depthBiasSlopeFactor;
  raster.depthBiasClamp = depthBiasClamp;
  raster.flags = flags;
  return raster;
}


VkPipelineColorBlendAttachmentState CreateColorBlendAttachmentState(VkBool32 blendEnable, 
  VkColorComponentFlags colorWriteMask, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor,
  VkBlendOp colorBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor,
  VkBlendOp alphaBlendOp)
{
  VkPipelineColorBlendAttachmentState blend = { };
  blend.colorWriteMask = colorWriteMask;
  blend.blendEnable = blendEnable;
  blend.srcColorBlendFactor = srcColorBlendFactor;
  blend.dstColorBlendFactor = dstColorBlendFactor;
  blend.colorBlendOp = colorBlendOp;
  blend.srcAlphaBlendFactor = srcAlphaBlendFactor;
  blend.dstAlphaBlendFactor = dstAlphaBlendFactor;
  blend.alphaBlendOp = alphaBlendOp;
  return blend;
}


VkPipelineColorBlendStateCreateInfo CreateBlendStateInfo(u32 attachmentCount,
  const VkPipelineColorBlendAttachmentState* pAttachments, VkBool32 logicOpEnable,
  VkLogicOp logicOp, r32 constant0 = 0.0f, r32 constant1 = 0.0f, 
  r32 constant2 = 0.0f, r32 constant3 = 0.0f, VkPipelineColorBlendStateCreateFlags flags = 0)
{
  VkPipelineColorBlendStateCreateInfo blend =  { };
  blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; 
  blend.attachmentCount = attachmentCount;
  blend.pAttachments = pAttachments;
  blend.logicOpEnable = logicOpEnable;
  blend.logicOp = logicOp;
  blend.blendConstants[0] = constant0;
  blend.blendConstants[1] = constant1;
  blend.blendConstants[2] = constant2;
  blend.blendConstants[3] = constant3;
  blend.flags = flags;
  return blend;
}


inline VkIndexType GetNativeIndexType(u32 sizeType)
{
  VkIndexType indexType;
  switch (sizeType) {
  case INDEX_SIZE_TYPE_16_BIT: indexType = VK_INDEX_TYPE_UINT16; break;
  case INDEX_SIZE_TYPE_32_BIT:
  default:  indexType = VK_INDEX_TYPE_UINT32; break;
  }
  return indexType;
}
}
} // Recluse