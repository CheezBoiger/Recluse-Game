// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>


#define NVIDIA_VENDOR_ID        0x10DE
#define INTEL_VENDOR_ID         0x1F96
#define AMD_VENDOR_ID           0x1022

class VulkanHandle {
public:
  VulkanHandle()
    : mOwner(VK_NULL_HANDLE) { }

  void      SetOwner(VkDevice owner) { mOwner = owner; }
  VkDevice  Owner() { return mOwner; }

protected:
  VkDevice  mOwner;
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


VkRenderPassCreateInfo CreateRenderPassInfo(Recluse::u32 attachmentCount, Recluse::u32 dependencyCount, 
  VkAttachmentDescription* pAttachments, VkSubpassDependency* pDependencies, 
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
}
} // Recluse