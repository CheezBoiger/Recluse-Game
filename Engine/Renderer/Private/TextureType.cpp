// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TextureType.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/LogicalDevice.hpp"

#include "Core/Math/Common.hpp"
#include "Core/Utility/Image.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


std::string TextureBase::kDefaultName = "DefaultTexture_";
u64         TextureBase::sIteration = 0;


void Texture2D::Initialize(u32 width, u32 height, b32 genMips)
{
  if (texture) return;

  texture = mRhi->CreateTexture();

  VkImageCreateInfo imgCI = { };
  VkImageViewCreateInfo imgViewCI = { };
  
  u32 mips = (!genMips ? 1 : u32((Log2f(static_cast<r32>(R_Max(width, height)) + 1))));
  imgCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCI.mipLevels = mips;
  imgCI.arrayLayers = 1;
  imgCI.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCI.imageType = VK_IMAGE_TYPE_2D;
  imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCI.extent.width = width;
  imgCI.extent.height = height;
  imgCI.extent.depth = 1;
  imgCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  
  imgViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
  imgViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imgViewCI.subresourceRange = { };
  imgViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgViewCI.subresourceRange.baseArrayLayer = 0;
  imgViewCI.subresourceRange.baseMipLevel = 0;
  imgViewCI.subresourceRange.layerCount = 1;
  imgViewCI.subresourceRange.levelCount = mips;
  imgViewCI.components = { };
  imgViewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
  
  texture->Initialize(imgCI, imgViewCI);
}


void Texture2D::CleanUp()
{
  if (texture) {
    mRhi->FreeTexture(texture);
    texture = nullptr;
  }
}


void Texture2D::Update(Image const& Image)
{
  VkDeviceSize imageSize = Image.MemorySize();
  Buffer stagingBuffer;
  stagingBuffer.SetOwner( mRhi->LogicDevice()->Native() );

  VkBufferCreateInfo stagingCI = {};
  stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingCI.size = imageSize;
  stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  stagingBuffer.Initialize(stagingCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VkResult result = stagingBuffer.Map();
  memcpy(stagingBuffer.Mapped(), Image.Data(), imageSize);
  stagingBuffer.UnMap();

  CommandBuffer buffer;
  buffer.SetOwner(mRhi->LogicDevice()->Native());
  buffer.Allocate(mRhi->TransferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  // Max barriers.
  std::vector<VkBufferImageCopy> bufferCopies(texture->MipLevels());
  size_t offset = 0;
  for (u32 mipLevel = 0; mipLevel < texture->MipLevels(); ++mipLevel) {
    VkBufferImageCopy region = { };
    region.bufferOffset = offset;
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageExtent.width = texture->Width();
    region.imageExtent.height = texture->Height();
    region.imageExtent.depth = 1;
    region.imageOffset = { 0, 0, 0 };
    bufferCopies[mipLevel] = region;
  }

  VkImageMemoryBarrier imgBarrier = {};
  imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imgBarrier.image = texture->Image();
  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgBarrier.srcAccessMask = 0;
  imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgBarrier.subresourceRange.baseArrayLayer = 0;
  imgBarrier.subresourceRange.baseMipLevel = 0;
  imgBarrier.subresourceRange.layerCount = texture->ArrayLayers();
  imgBarrier.subresourceRange.levelCount = texture->MipLevels();

  // TODO(): Copy buffer to image stream.
  buffer.Begin(beginInfo);
  // Image memory barrier.
  buffer.PipelineBarrier(
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  // Send buffer image copy cmd.
  buffer.CopyBufferToImage(
    stagingBuffer.NativeBuffer(),
    texture->Image(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    static_cast<u32>(bufferCopies.size()),
    bufferCopies.data()
  );

  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  buffer.PipelineBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  buffer.End();

  // TODO(): Submit it to graphics queue!
  VkCommandBuffer commandbuffers[] = { buffer.Handle() };

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = commandbuffers;

  mRhi->TransferSubmit(DEFAULT_QUEUE_IDX, 1, &submit);
  mRhi->TransferWaitIdle(DEFAULT_QUEUE_IDX);

  buffer.Free();
  stagingBuffer.CleanUp();
}


u32 Texture2D::Width() const 
{
  if (!texture) return 0; 
  return texture->Width();
}


u32 Texture2D::Height() const
{
  if (!texture) return 0;
  return texture->Height();
}
} // Recluse