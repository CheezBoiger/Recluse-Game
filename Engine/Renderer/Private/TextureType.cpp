// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TextureType.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Texture.hpp"

#include "Core/Math/Common.hpp"
#include "Core/Utility/Image.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void Texture2D::Initialize(Image const& image)
{
  if (texture) return;

  texture = mRhi->CreateTexture();

  VkImageCreateInfo imgCI = { };
  VkImageViewCreateInfo imgViewCI = { };

  imgCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCI.mipLevels = 1;//log2f(Max(image->Width, image->Height)) + 1;
  imgCI.arrayLayers = 1;
  imgCI.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCI.imageType = VK_IMAGE_TYPE_2D;
  imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCI.extent.width = image.Width();
  imgCI.extent.height = image.Height();
  imgCI.extent.depth = 1;
  imgCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  imgViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
  imgViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imgViewCI.subresourceRange = { };
  imgViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgViewCI.subresourceRange.baseArrayLayer = 0;
  imgViewCI.subresourceRange.baseMipLevel = 0;
  imgViewCI.subresourceRange.layerCount = 1;
  imgViewCI.subresourceRange.levelCount = 1;
  imgViewCI.components = { };
  imgViewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
  
  texture->Initialize(imgCI, imgViewCI);
  texture->Upload(mRhi, image);
}


void Texture2D::CleanUp()
{
  if (texture) {
    mRhi->FreeTexture(texture);
    texture = nullptr;
  }
}


void Texture2D::Update(Image const& image)
{
  if (texture) texture->Upload(mRhi, image);
}
} // Recluse