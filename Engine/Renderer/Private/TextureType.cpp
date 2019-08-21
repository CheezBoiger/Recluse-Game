// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TextureType.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Texture.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/LogicalDevice.hpp"

#include "Renderer.hpp"

#include "Core/Math/Common.hpp"
#include "Core/Utility/Image.hpp"
#include "Core/Exception.hpp"

#include <array>

namespace Recluse {


std::string TextureBase::kDefaultName = "DefaultTexture_";
U64         TextureBase::sIteration = 0;
UUID64 TextureSampler::sIteration = 0;


VkFormat GetNativeFormat(RFormat format)
{
  switch (format) {
    case RFORMAT_R16G16_UNORM: return VK_FORMAT_R16G16_UNORM;
    case RFORMAT_R8G8B8A8_UNORM:
    default: return VK_FORMAT_R8G8B8A8_UNORM;
  }
}


void Texture2D::initialize(RFormat format, U32 width, U32 height, B32 genMips)
{
  if (texture) return;

  texture = mRhi->createTexture();
  m_bGenMips = genMips;

  VkImageCreateInfo imgCI = { };
  VkImageViewCreateInfo imgViewCI = { };
  
  U32 mips = (!genMips ? 1 : U32((Log2f(static_cast<R32>(R_Max(width, height)) + 1))));
  imgCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCI.mipLevels = mips;
  imgCI.arrayLayers = 1;
  imgCI.format = GetNativeFormat(format);
  imgCI.imageType = VK_IMAGE_TYPE_2D;
  imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCI.extent.width = width;
  imgCI.extent.height = height;
  imgCI.extent.depth = 1;
  imgCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (genMips) {
    imgCI.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }
  
  imgViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
  imgViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imgViewCI.subresourceRange = { };
  imgViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgViewCI.subresourceRange.baseArrayLayer = 0;
  imgViewCI.subresourceRange.baseMipLevel = 0;
  imgViewCI.subresourceRange.layerCount = 1;
  imgViewCI.subresourceRange.levelCount = mips;
  imgViewCI.components = { };
  imgViewCI.format = GetNativeFormat(format);
  
  texture->initialize(imgCI, imgViewCI);
}


void Texture2D::cleanUp()
{
  if (texture) {
    mRhi->freeTexture(texture);
    texture = nullptr;
  }
}


void GenerateMipMaps(VkImage image, VkFormat format, U32 width, U32 height, U32 mipLevels)
{
  {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(VulkanRHI::gPhysicalDevice.handle(), format, &properties);
    if (!(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
      R_DEBUG(rError, "Texture image format on this GPU does not support linear blitting. Skipping mipmap gen.\n");
      return;
    }
  }

  CommandBuffer cmdBuffer; 
  cmdBuffer.SetOwner(gRenderer().getRHI()->logicDevice()->getNative());
  cmdBuffer.allocate(gRenderer().getRHI()->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  
  {
    VkCommandBufferBeginInfo begin = { };
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdBuffer.begin(begin);
  }

  VkImageMemoryBarrier barrier = { };
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; 
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  U32 mipWidth = width;
  U32 mipHeight = height;

  for (U32 i = 1; i < mipLevels; ++i) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    cmdBuffer.pipelineBarrier(
      VK_PIPELINE_STAGE_TRANSFER_BIT, 
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    VkImageBlit blit= { };
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { I32(mipWidth), I32(mipHeight), 1 };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.srcSubresource.mipLevel = i - 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { mipWidth > 1 ? I32(mipWidth / 2) : 1, mipHeight > 1 ? I32(mipHeight / 2) : 1, 1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    blit.dstSubresource.mipLevel = i;

    cmdBuffer.imageBlit(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit, VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    cmdBuffer.pipelineBarrier(
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0, 
      0, nullptr,
      0, nullptr,
      1, &barrier);
    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  
  cmdBuffer.pipelineBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
  
  cmdBuffer.end();
  
  VkSubmitInfo submit = { };
  VkCommandBuffer cmd[] = { cmdBuffer.getHandle() };
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = cmd;
  
  gRenderer().getRHI()->graphicsSubmit(DEFAULT_QUEUE_IDX, 1, &submit);
  gRenderer().getRHI()->graphicsWaitIdle(DEFAULT_QUEUE_IDX);
}


void Texture2D::update(Image const& Image)
{
  VkDeviceSize imageSize = Image.MemorySize();
  Buffer stagingBuffer;
  stagingBuffer.SetOwner( mRhi->logicDevice()->getNative() );

  VkBufferCreateInfo stagingCI = {};
  stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingCI.size = imageSize;
  stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  stagingBuffer.initialize(stagingCI, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU);

  VkResult result = stagingBuffer.map();
  memcpy(stagingBuffer.getMapped(), Image.getData(), imageSize);
  stagingBuffer.unmap();

  CommandBuffer buffer;
  buffer.SetOwner(mRhi->logicDevice()->getNative());
  buffer.allocate(mRhi->transferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  // maximum barriers.
  std::vector<VkBufferImageCopy> bufferCopies(1);
  size_t offset = 0;
  for (U32 mipLevel = 0; mipLevel < 1; ++mipLevel) {
    VkBufferImageCopy region = { };
    region.bufferOffset = offset;
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageExtent.width = texture->getWidth();
    region.imageExtent.height = texture->getHeight();
    region.imageExtent.depth = 1;
    region.imageOffset = { 0, 0, 0 };
    bufferCopies[mipLevel] = region;
  }

  VkImageMemoryBarrier imgBarrier = {};
  imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imgBarrier.image = texture->getImage();
  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgBarrier.srcAccessMask = 0;
  imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgBarrier.subresourceRange.baseArrayLayer = 0;
  imgBarrier.subresourceRange.baseMipLevel = 0;
  imgBarrier.subresourceRange.layerCount = texture->getArrayLayers();
  imgBarrier.subresourceRange.levelCount = texture->getMipLevels();

  // TODO(): Copy buffer to image stream.
  buffer.begin(beginInfo);
  // Image memory barrier.
  buffer.pipelineBarrier(
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  // Send buffer image copy cmd.
  buffer.copyBufferToImage(
    stagingBuffer.getNativeBuffer(),
    texture->getImage(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    static_cast<U32>(bufferCopies.size()),
    bufferCopies.data()
  );

  if (!m_bGenMips) {
    imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    buffer.pipelineBarrier(
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &imgBarrier
    );
  }
  buffer.end();

  // TODO(): Submit it to graphics queue!
  VkCommandBuffer commandbuffers[] = { buffer.getHandle() };

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = commandbuffers;

  mRhi->transferSubmit(DEFAULT_QUEUE_IDX, 1, &submit);
  mRhi->transferWaitIdle(DEFAULT_QUEUE_IDX);

  buffer.free();
  stagingBuffer.cleanUp();

  if (m_bGenMips) {
    GenerateMipMaps(texture->getImage(), 
      texture->getFormat(), 
      texture->getWidth(), 
      texture->getHeight(), 
      texture->getMipLevels());
  } 
}


U32 Texture2D::getWidth() const 
{
  if (!texture) return 0; 
  return texture->getWidth();
}


U32 Texture2D::getHeight() const
{
  if (!texture) return 0;
  return texture->getHeight();
}


void Texture2D::save(const std::string filename) 
{
  CommandBuffer cmdBuffer;
  cmdBuffer.SetOwner(mRhi->logicDevice()->getNative());
  cmdBuffer.allocate(mRhi->transferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  Buffer stagingBuffer;
  stagingBuffer.SetOwner(mRhi->logicDevice()->getNative());

  VkBufferCreateInfo bufferci = {};
  bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferci.size = VkDeviceSize(texture->getWidth() * texture->getHeight() * 4);
  U8* data = new U8[bufferci.size];


  stagingBuffer.initialize(bufferci, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU);
  stagingBuffer.map();

  // Stream out the image info.
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmdBuffer.begin(beginInfo);

  // maximum barriers.
  std::vector<VkBufferImageCopy> bufferCopies(texture->getMipLevels());
  size_t offset = 0;
  for (U32 mipLevel = 0; mipLevel < texture->getMipLevels(); ++mipLevel) {
    VkBufferImageCopy region = {};
    region.bufferOffset = offset;
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageExtent.width = texture->getWidth();
    region.imageExtent.height = texture->getHeight();
    region.imageExtent.depth = 1;
    region.imageOffset = { 0, 0, 0 };
    bufferCopies[mipLevel] = region;
  }

  VkImageMemoryBarrier imgBarrier = {};
  imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imgBarrier.image = texture->getImage();
  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  imgBarrier.srcAccessMask = 0;
  imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgBarrier.subresourceRange.baseArrayLayer = 0;
  imgBarrier.subresourceRange.baseMipLevel = 0;
  imgBarrier.subresourceRange.layerCount = texture->getArrayLayers();
  imgBarrier.subresourceRange.levelCount = texture->getMipLevels();

  // TODO(): Copy buffer to image stream.
  // Image memory barrier.
  cmdBuffer.pipelineBarrier(
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  // Send buffer image copy cmd.
  cmdBuffer.copyImageToBuffer(
    texture->getImage(),
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    stagingBuffer.getNativeBuffer(),
    static_cast<U32>(bufferCopies.size()),
    bufferCopies.data()
  );

  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  cmdBuffer.pipelineBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  cmdBuffer.end();

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  VkCommandBuffer cmd[] = { cmdBuffer.getHandle() };
  submit.pCommandBuffers = cmd;

  mRhi->transferSubmit(0, 1, &submit);
  mRhi->transferWaitIdle(0);

  memcpy(data, stagingBuffer.getMapped(), bufferci.size);
  Image img;
  img._data = data;
  img._height = texture->getHeight();
  img._width = texture->getWidth();
  img._channels = 4;
  img._memorySize = texture->getWidth() * texture->getHeight() * 4;
  img.SavePNG(filename.c_str());

  stagingBuffer.unmap();
  stagingBuffer.cleanUp();
  delete[] data;
}


void TextureCube::initialize(U32 dim)
{
  if (texture) return;

  VkImageCreateInfo imageCi = { };
  imageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCi.extent.width = dim;
  imageCi.extent.height = dim;
  imageCi.extent.depth = 1;
  imageCi.arrayLayers = 6;
  imageCi.imageType = VK_IMAGE_TYPE_2D;
  imageCi.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  imageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageCi.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCi.mipLevels = 1;
  imageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCi.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImageViewCreateInfo viewCi = { };
  viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCi.components = { };
  viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewCi.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewCi.subresourceRange.baseArrayLayer = 0;
  viewCi.subresourceRange.baseMipLevel = 0;
  viewCi.subresourceRange.layerCount = 6;
  viewCi.subresourceRange.levelCount = 1;

  texture = mRhi->createTexture();
  texture->initialize(imageCi, viewCi);
}


U32 TextureCube::WidthPerFace() const
{
  return texture->getWidth();
}


U32 TextureCube::HeightPerFace() const
{
  return texture->getHeight();
}


void TextureCube::cleanUp()
{
  // TODO(): 
  if ( texture ) {
    mRhi->freeTexture( texture );
    texture = nullptr;
  }
}


void TextureCube::save(const std::string filename)
{
  CommandBuffer cmdBuffer;
  cmdBuffer.SetOwner(mRhi->logicDevice()->getNative());
  cmdBuffer.allocate(mRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  // 6 textures.
  std::vector<VkBufferImageCopy> imageCopyRegions;
  VkDeviceSize offset = 0;

  for (size_t layer = 0; layer < texture->getArrayLayers(); ++layer) {
    for (size_t level = 0; level < texture->getMipLevels(); ++level) {
      VkBufferImageCopy region = { };
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.baseArrayLayer = (U32)layer;
      region.imageSubresource.layerCount = 1;
      region.imageSubresource.mipLevel = (U32)level;
      region.imageExtent.width = texture->getWidth();
      region.imageExtent.height = texture->getHeight();
      region.imageExtent.depth = 1;
      region.bufferOffset = offset;
      imageCopyRegions.push_back(region);
      offset += texture->getWidth() * texture->getHeight() * 4;
    }
  }

  VkDeviceSize sizeInBytes = offset;

  Buffer stagingBuffer;
  stagingBuffer.SetOwner(mRhi->logicDevice()->getNative());

  VkBufferCreateInfo bufferci = {};
  bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferci.size = sizeInBytes;
  U8* data = new U8[bufferci.size];

  stagingBuffer.initialize(bufferci, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU);
  stagingBuffer.map();

  VkCommandBufferBeginInfo begin = {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmdBuffer.reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  cmdBuffer.begin(begin);

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
  imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imgMemBarrier.srcAccessMask = 0;
  imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgMemBarrier.image = texture->getImage();

  // set the cubemap image layout for transfer from our framebuffer.
  cmdBuffer.pipelineBarrier(
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgMemBarrier
  );

////////////////////////////
  cmdBuffer.copyImageToBuffer(texture->getImage(),
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    stagingBuffer.getNativeBuffer(),
    static_cast<U32>(imageCopyRegions.size()),
    imageCopyRegions.data());

  subRange.baseMipLevel = 0;
  subRange.baseArrayLayer = 0;
  subRange.levelCount = 1;
  subRange.layerCount = 6;

  imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  imgMemBarrier.image = texture->getImage();
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

  VkCommandBuffer cmd[] = { cmdBuffer.getHandle() };
  VkSubmitInfo submit = { };
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = cmd;

  mRhi->graphicsSubmit(0, 1, &submit);
  mRhi->graphicsWaitIdle(0);

  memcpy(data, stagingBuffer.getMapped(), sizeInBytes);

  Image img;
  img._data = data;
  img._height = texture->getHeight() * 6;
  img._width = texture->getWidth();
  img._channels = 4;
  img._memorySize = U32(sizeInBytes);
  img.SavePNG(filename.c_str());

  stagingBuffer.unmap();
  stagingBuffer.cleanUp();
  delete[] data;
}


VkSamplerAddressMode GetNativeSamplerAddressMode(SamplerAddressMode mode)
{
  switch (mode) {
    case SAMPLER_ADDRESS_CLAMP_TO_BORDER:       return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case SAMPLER_ADDRESS_CLAMP_TO_EDGE:         return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case SAMPLER_ADDRESS_MIRRORED_REPEAT:       return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case SAMPLER_ADDRESS_MIRROR_CLAMP_TO_EDGE:  return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    case SAMPLER_ADDRESS_REPEAT:                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    default:                                    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  }
  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
}


VkFilter GetNativeFilterMode(SamplerFilterMode mode)
{
  switch (mode) {
    case SAMPLER_FILTER_NEAREST:  return VK_FILTER_NEAREST;
    case SAMPLER_FILTER_LINEAR:
    default:                      return VK_FILTER_LINEAR;
  }
}


VkSamplerMipmapMode GetNativeMipmapMode(SamplerMipMapMode mode)
{
  switch (mode) {
    case SAMPLER_MIPMAP_MODE_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case SAMPLER_MIPMAP_MODE_LINEAR:  
    default:                          return VK_SAMPLER_MIPMAP_MODE_LINEAR;
  }
}


VkBorderColor GetNativeSamplerBorderColor(SamplerBorderColor color)
{
  switch (color) {
    case SAMPLER_BORDER_COLOR_OPAQUE_BLACK:   return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case SAMPLER_BORDER_COLOR_OPAQUE_WHITE:
    default:                                  return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  }
}


void TextureSampler::initialize(VulkanRHI* pRhi, const SamplerInfo& info)
{
  mInfo = info;
  VkSamplerCreateInfo samplerCi = { };
  samplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;  
  samplerCi.addressModeU = GetNativeSamplerAddressMode(info._addrU);
  samplerCi.addressModeV = GetNativeSamplerAddressMode(info._addrV);
  samplerCi.addressModeW = GetNativeSamplerAddressMode(info._addrW);
  samplerCi.borderColor = GetNativeSamplerBorderColor(info._borderColor);
  samplerCi.mipmapMode = GetNativeMipmapMode(info._mipmapMode);
  samplerCi.unnormalizedCoordinates = info._unnnormalizedCoordinates;
  samplerCi.anisotropyEnable = info._enableAnisotropy;
  samplerCi.compareEnable = VK_FALSE;
  samplerCi.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerCi.magFilter = GetNativeFilterMode(info._maxFilter);
  samplerCi.minFilter = GetNativeFilterMode(info._minFilter);
  samplerCi.mipLodBias = info._mipLodBias;
  samplerCi.minLod = info._minLod;
  samplerCi.maxLod = info._maxLod;
  samplerCi.maxAnisotropy = info._maxAniso;

  mSampler = pRhi->createSampler();
  mSampler->initialize(samplerCi);
}


void TextureSampler::cleanUp(VulkanRHI* pRhi)
{
  if (mSampler) {
    pRhi->freeSampler(mSampler);
    mSampler = nullptr;
  }
}


void TextureCube::update(Image const& image)
{
  U32 width = image.getWidth();
  U32 heightOffset = image.getHeight() / 6;

  CommandBuffer cmdBuffer;
  cmdBuffer.SetOwner(mRhi->logicDevice()->getNative());
  cmdBuffer.allocate(mRhi->graphicsCmdPool(0), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  // 6 textures.
  std::vector<VkBufferImageCopy> imageCopyRegions;
  VkDeviceSize offset = 0;

  for (size_t layer = 0; layer < texture->getArrayLayers(); ++layer) {
    for (size_t level = 0; level < texture->getMipLevels(); ++level) {
      VkBufferImageCopy region = {};
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.baseArrayLayer = (U32)layer;
      region.imageSubresource.layerCount = 1;
      region.imageSubresource.mipLevel = (U32)level;
      region.imageExtent.width = texture->getWidth();
      region.imageExtent.height = heightOffset;
      region.imageExtent.depth = 1;
      region.bufferOffset = offset;
      imageCopyRegions.push_back(region);
      offset += texture->getWidth() * heightOffset * 4;
    }
  }

  VkDeviceSize sizeInBytes = offset;

  Buffer stagingBuffer;
  stagingBuffer.SetOwner(mRhi->logicDevice()->getNative());

  VkBufferCreateInfo bufferci = {};
  bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferci.size = sizeInBytes;

  stagingBuffer.initialize(bufferci, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU);
  stagingBuffer.map();
  memcpy(stagingBuffer.getMapped(), image.getData(), sizeInBytes);

  VkCommandBufferBeginInfo begin = {};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  cmdBuffer.reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  cmdBuffer.begin(begin);

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
  imgMemBarrier.image = texture->getImage();

  // set the cubemap image layout for transfer from our framebuffer.
  cmdBuffer.pipelineBarrier(
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgMemBarrier
  );

  ////////////////////////////
  cmdBuffer.copyBufferToImage(stagingBuffer.getNativeBuffer(),
    texture->getImage(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    static_cast<U32>(imageCopyRegions.size()),
    imageCopyRegions.data());

  subRange.baseMipLevel = 0;
  subRange.baseArrayLayer = 0;
  subRange.levelCount = 1;
  subRange.layerCount = 6;

  imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  imgMemBarrier.image = texture->getImage();
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

  VkCommandBuffer cmd[] = { cmdBuffer.getHandle() };
  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = cmd;

  mRhi->graphicsSubmit(0, 1, &submit);
  mRhi->graphicsWaitIdle(0);

  stagingBuffer.unmap();
  stagingBuffer.cleanUp();
}


void TextureCubeArray::initialize(U32 dim, U32 cubeLayers)
{
  VkImageCreateInfo imgCi = { };
  imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;  
  imgCi.arrayLayers = 6 * cubeLayers;
  imgCi.extent.width = dim;
  imgCi.extent.height = dim;
  imgCi.extent.depth = 1;
  imgCi.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;  
  imgCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCi.imageType = VK_IMAGE_TYPE_2D;
  imgCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCi.mipLevels = 1;
  imgCi.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCi.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  VkImageViewCreateInfo viewCi = { };
  viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCi.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewCi.subresourceRange.baseArrayLayer = 0;
  viewCi.subresourceRange.baseMipLevel = 0;
  viewCi.subresourceRange.layerCount = 6 * cubeLayers;
  viewCi.subresourceRange.levelCount = 1;
  viewCi.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;;
  viewCi.components = { };

  texture = mRhi->createTexture();
  texture->initialize(imgCi, viewCi);  
}


void TextureCubeArray::cleanUp()
{
  if (texture) {
    mRhi->freeTexture(texture);
    texture = nullptr;
  }
}


void Texture2DArray::initialize(RFormat format, U32 width, U32 height, U32 layers)
{
  if (texture) return;

  texture = mRhi->createTexture();

  VkImageCreateInfo imgCI = {};
  VkImageViewCreateInfo imgViewCI = {};

  U32 mips = 1;//(!genMips ? 1 : U32((Log2f(static_cast<R32>(R_Max(width, height)) + 1))));
  imgCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCI.mipLevels = mips;
  imgCI.arrayLayers = layers;
  imgCI.format = GetNativeFormat(format);
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
  imgViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  imgViewCI.subresourceRange = {};
  imgViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgViewCI.subresourceRange.baseArrayLayer = 0;
  imgViewCI.subresourceRange.baseMipLevel = 0;
  imgViewCI.subresourceRange.layerCount = layers;
  imgViewCI.subresourceRange.levelCount = mips;
  imgViewCI.components = {};
  imgViewCI.format = GetNativeFormat(format);

  texture->initialize(imgCI, imgViewCI);
}


void Texture2DArray::update(const Image& img, U32 x, U32 y)
{
  // tODO(): NEed to figure out why image is not being read properly,
  // Only reading the first 4 slices and repeating!
  VkDeviceSize imageSize = img.MemorySize();
  Buffer stagingBuffer;
  stagingBuffer.SetOwner(mRhi->logicDevice()->getNative());
  
  U32 widthOffset = img.getWidth() / x;
  U32 heightOffset = img.getHeight() / y;

  VkBufferCreateInfo stagingCI = {};
  stagingCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingCI.size = imageSize;
  stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  stagingBuffer.initialize(stagingCI, PHYSICAL_DEVICE_MEMORY_USAGE_CPU_TO_GPU);

  VkResult result = stagingBuffer.map();
  memcpy(stagingBuffer.getMapped(), img.getData(), imageSize);
  stagingBuffer.unmap();

  CommandBuffer buffer;
  buffer.SetOwner(mRhi->logicDevice()->getNative());
  buffer.allocate(mRhi->transferCmdPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  // maximum barriers.
  std::vector<VkBufferImageCopy> bufferCopies(texture->getArrayLayers());
  size_t offset = 0;
  U32 layer = 0;

  for (U32 yi = 0; yi < y; ++yi) {
    offset = heightOffset * img.getWidth() * 4 * yi;
    for (U32 xi = 0; xi < x; ++xi) {
      VkBufferImageCopy region = {};
      region.bufferOffset = offset;
      region.bufferImageHeight = 0;
      region.bufferRowLength = img.getWidth();
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.baseArrayLayer = layer;
      region.imageSubresource.layerCount = 1;
      region.imageSubresource.mipLevel = 0;
      region.imageExtent.width =  texture->getWidth();
      region.imageExtent.height = texture->getHeight();
      region.imageExtent.depth = 1;
      region.imageOffset = { 0, 0, 0 };
      bufferCopies[layer++] = region;
      offset += widthOffset * 4;
    }
  }
  VkImageMemoryBarrier imgBarrier = {};
  imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imgBarrier.image = texture->getImage();
  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgBarrier.srcAccessMask = 0;
  imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imgBarrier.subresourceRange.baseArrayLayer = 0;
  imgBarrier.subresourceRange.baseMipLevel = 0;
  imgBarrier.subresourceRange.layerCount = texture->getArrayLayers();
  imgBarrier.subresourceRange.levelCount = texture->getMipLevels();

  // TODO(): Copy buffer to image stream.
  buffer.begin(beginInfo);
  // Image memory barrier.
  buffer.pipelineBarrier(
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  // Send buffer image copy cmd.
  buffer.copyBufferToImage(
    stagingBuffer.getNativeBuffer(),
    texture->getImage(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    static_cast<U32>(bufferCopies.size()),
    bufferCopies.data()
  );

  imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  buffer.pipelineBarrier(
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &imgBarrier
  );

  buffer.end();

  // TODO(): Submit it to graphics queue!
  VkCommandBuffer commandbuffers[] = { buffer.getHandle() };

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = commandbuffers;

  mRhi->transferSubmit(DEFAULT_QUEUE_IDX, 1, &submit);
  mRhi->transferWaitIdle(DEFAULT_QUEUE_IDX);

  buffer.free();
  stagingBuffer.cleanUp();
}


void Texture2DArray::cleanUp()
{
  if (texture) {
    mRhi->freeTexture(texture);
    texture = nullptr;
  }
}
} // Recluse