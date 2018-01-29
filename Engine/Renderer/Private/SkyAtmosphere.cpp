// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "SkyAtmosphere.hpp"

#include "Core/Logging/Log.hpp"

#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/Framebuffer.hpp"

#include "RendererData.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


const std::string Sky::kVertStr = "Atmosphere.vert.spv";
const std::string Sky::kFragStr = "Atmosphere.frag.spv";
const u32         Sky::kTextureSize = 1024;

// Submit information, used for rendering.
VkSubmitInfo      submit =  { };

void Sky::Initialize()
{
  VulkanRHI* pRhi = gRenderer().RHI();

  CreateRenderAttachments(pRhi);
  CreateCubeMap(pRhi);
  CreateGraphicsPipeline(pRhi);
  CreateCommandBuffer(pRhi);
  BuildCmdBuffer(pRhi);

  m_pAtmosphereSema = pRhi->CreateVkSemaphore();
  VkSemaphoreCreateInfo semaCi = { };
  semaCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  m_pAtmosphereSema->Initialize(semaCi);

  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkCommandBuffer cmdbuf = m_pCmdBuffer->Handle();
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &cmdbuf;
  submit.signalSemaphoreCount = 1;
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = nullptr;
  VkSemaphore signal = m_pAtmosphereSema->Handle();
  submit.pSignalSemaphores = &signal;
}


Sky::~Sky()
{
  if (m_pCubeMap) {
    R_DEBUG(rError, "Skybox cube map was not properly cleaned up prioer to class descruction!\n");
  }

  for (auto& texture : m_RenderTextures) {
    if (texture) {
      R_DEBUG(rError, "Sky texture was not cleaned up prior to class destruction!\n");
    }
  }
}


void Sky::CreateCommandBuffer(VulkanRHI* rhi)
{
  m_pCmdBuffer = rhi->CreateCommandBuffer();
  m_pCmdBuffer->Allocate(rhi->GraphicsCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}


void Sky::CreateRenderAttachments(VulkanRHI* rhi)
{
  VkExtent2D extent = rhi->SwapchainObject()->SwapchainExtent();
  VkImageCreateInfo imgCi = { };
  imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCi.imageType = VK_IMAGE_TYPE_2D;
  imgCi.mipLevels = 1;
  imgCi.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCi.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  imgCi.extent.width = kTextureSize;
  imgCi.extent.height = kTextureSize;
  imgCi.extent.depth = 1;
  imgCi.arrayLayers = 1;
  
  VkImageViewCreateInfo viewCi = { };
  viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewCi.subresourceRange.baseArrayLayer = 0;
  viewCi.subresourceRange.baseMipLevel = 0;
  viewCi.subresourceRange.levelCount = 1;
  viewCi.subresourceRange.layerCount = 1;
  viewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;

  for (size_t i = 0; i < m_RenderTextures.size(); ++i) {
    m_RenderTextures[i] = rhi->CreateTexture();
    m_RenderTextures[i]->Initialize(imgCi, viewCi);
  }
} 


void Sky::CreateCubeMap(VulkanRHI* rhi)
{
  VkExtent2D extent = rhi->SwapchainObject()->SwapchainExtent();
  VkImageCreateInfo imgCi = {};
  imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCi.imageType = VK_IMAGE_TYPE_2D;
  imgCi.mipLevels = 1;
  imgCi.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imgCi.extent.width = kTextureSize;
  imgCi.extent.height = kTextureSize;
  imgCi.extent.depth = 1;
  imgCi.arrayLayers = 6;
  imgCi.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

  VkImageViewCreateInfo viewCi = {};
  viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewCi.subresourceRange.baseArrayLayer = 0;
  viewCi.subresourceRange.baseMipLevel = 0;
  viewCi.subresourceRange.levelCount = 1;
  viewCi.subresourceRange.layerCount = 6;
  viewCi.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

  m_pCubeMap = rhi->CreateTexture();
  m_pCubeMap->Initialize(imgCi, viewCi);

  // Now we create the samplerCube.
  m_pSampler = rhi->CreateSampler();
  VkSamplerCreateInfo samplerCi = { };
  samplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCi.anisotropyEnable = VK_TRUE;
  samplerCi.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerCi.compareEnable = VK_FALSE;
  samplerCi.compareOp = VK_COMPARE_OP_NEVER;
  samplerCi.magFilter = VK_FILTER_LINEAR;
  samplerCi.minFilter = VK_FILTER_LINEAR;
  samplerCi.maxAnisotropy = 16.0f;
  samplerCi.maxLod = 1.0f;
  samplerCi.minLod = 0.0f;
  samplerCi.mipLodBias = 0.0f;
  samplerCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCi.unnormalizedCoordinates = VK_FALSE;
  m_pSampler->Initialize(samplerCi);
}


void Sky::CreateGraphicsPipeline(VulkanRHI* rhi)
{
  
}


void Sky::BuildCmdBuffer(VulkanRHI* rhi)
{
  if (m_pCmdBuffer) {
    rhi->GraphicsWaitIdle();
    m_pCmdBuffer->Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  }
  
  VkCommandBufferBeginInfo begin = { };
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
 
  CommandBuffer* cmdBuffer = m_pCmdBuffer;
  cmdBuffer->Begin(begin);
  cmdBuffer->End();
}


void Sky::Render(Semaphore* signal)
{
  VulkanRHI* rhi = gRenderer().RHI();
  VkSemaphore wait = signal->Handle();
  submit.pWaitSemaphores = &wait;

  rhi->GraphicsSubmit(submit);
  rhi->DeviceWaitIdle();
  
  // Need to stream over rendered textures to cubemap.
  CommandBuffer cmdbuf;
  cmdbuf.SetOwner(rhi->LogicDevice()->Native());

  // One time command buffer to stream over textures.
}


void Sky::CleanUp()
{
  VulkanRHI* rhi = gRenderer().RHI();
  if (m_pCubeMap) {
    rhi->FreeTexture(m_pCubeMap);
    m_pCubeMap = nullptr;
    rhi->FreeSampler(m_pSampler);
    m_pSampler = nullptr;
  }

  if (m_pAtmosphereSema) {
    rhi->FreeVkSemaphore(m_pAtmosphereSema);
    m_pAtmosphereSema = nullptr;
  }

  if (m_pCmdBuffer) {
    rhi->FreeCommandBuffer(m_pCmdBuffer);
    m_pCmdBuffer = nullptr;
  }

  for (auto& texture : m_RenderTextures) {
    rhi->FreeTexture(texture);
    texture = nullptr;
  }
}
} // Recluse 