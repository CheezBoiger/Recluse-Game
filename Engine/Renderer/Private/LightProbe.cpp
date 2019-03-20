//
#include "LightProbe.hpp"
#include "TextureType.hpp"
#include "Renderer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RendererData.hpp"
#include "SkyAtmosphere.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/GraphicsPipeline.hpp"

#include "Core/Logging/Log.hpp"
#include "Filesystem/Filesystem.hpp"

#include <random>

namespace Recluse {


Vector3 GetWorldDirection(u32 faceIdx, u32 px, u32 py, u32 width, u32 height)
{
  Vector3 worldDir = Vector3();
  Vector2 pixelSize(1.0f / static_cast<r32>(width), 1.0f / static_cast<r32>(height));
  Vector2 uv = Vector2(static_cast<r32>(px) + 0.5f, static_cast<r32>(py) + 0.5f) * pixelSize;
  uv.y = 1.f - uv.y;
  uv -= 0.5f; // [-0.5, 0.5]

  switch (faceIdx) {
    case 0:   worldDir = Vector3( 0.5f,  uv.y, -uv.x); break;
    case 1:   worldDir = Vector3(-0.5f,  uv.y,  uv.x); break;
    case 2:   worldDir = Vector3( uv.x,  0.5f, -uv.y); break;
    case 3:   worldDir = Vector3( uv.x, -0.5f,  uv.y); break;
    case 4:   worldDir = Vector3( uv.x,  uv.y,  0.5f); break;
    case 5:   worldDir = Vector3(-uv.x,  uv.y, -0.5f); break;
    default:  worldDir = Vector3(); break;
  }
  return worldDir;
}


Vector3 GetWorldNormalFromCubeFace(u32 idx)
{
  // Get cube face normal.
  Vector3 n = Vector3(0.0f);
  n[idx >> 1] = (idx & 1) ? 1.0f : -1.0f;
  return n;
}


void LightProbe::generateSHCoefficients(VulkanRHI* rhi, TextureCube* envMap)
{
  u32 width = envMap->WidthPerFace();
  u32 height = envMap->HeightPerFace();
  r32 pixelA = (1.0f / static_cast<r32>(width)) * (1.0f / static_cast<r32>(height));
  u8* data = nullptr;

  {
    VkCommandBuffer cmdBuf;
    VkCommandBufferAllocateInfo allocInfo = {};
    Texture* texture = envMap->getHandle();
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = rhi->graphicsCmdPool(0);
    allocInfo.commandBufferCount = 1;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(rhi->logicDevice()->getNative(), &allocInfo, &cmdBuf);
    
    // Read image data through here.
    // TODO(): 
    // 6 textures.
    std::vector<VkBufferImageCopy> imageCopyRegions;
    VkDeviceSize offset = 0;

    for (size_t layer = 0; layer < texture->ArrayLayers(); ++layer) {
      for (size_t level = 0; level < texture->MipLevels(); ++level) {
        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.baseArrayLayer = (u32)layer;
        region.imageSubresource.layerCount = 1;
        region.imageSubresource.mipLevel = (u32)level;
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
    stagingBuffer.SetOwner(rhi->logicDevice()->getNative());

    VkBufferCreateInfo bufferci = {};
    bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferci.size = sizeInBytes;
    data = new u8[bufferci.size];

    stagingBuffer.initialize(bufferci, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.Map();

    VkCommandBufferBeginInfo begin = {};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    
    vkResetCommandBuffer(cmdBuf, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    vkBeginCommandBuffer(cmdBuf, &begin);

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
    imgMemBarrier.image = texture->Image();

    // set the cubemap image layout for transfer from our framebuffer.
    vkCmdPipelineBarrier(
      cmdBuf,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &imgMemBarrier
    );

    ////////////////////////////
    vkCmdCopyImageToBuffer(cmdBuf, texture->Image(),
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      stagingBuffer.NativeBuffer(),
      static_cast<u32>(imageCopyRegions.size()),
      imageCopyRegions.data());

    subRange.baseMipLevel = 0;
    subRange.baseArrayLayer = 0;
    subRange.levelCount = 1;
    subRange.layerCount = 6;

    imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imgMemBarrier.image = texture->Image();
    imgMemBarrier.subresourceRange = subRange;

    vkCmdPipelineBarrier(
      cmdBuf,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &imgMemBarrier
    );

    vkEndCommandBuffer(cmdBuf);

    VkSubmitInfo submit = { };
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    VkCommandBuffer cmdB[] = { cmdBuf };
    submit.pCommandBuffers = cmdB;
    rhi->graphicsSubmit(0, 1, &submit);
    rhi->graphicsWaitIdle(0);

    memcpy(data, stagingBuffer.Mapped(), sizeInBytes);

    vkFreeCommandBuffers(rhi->logicDevice()->getNative(), rhi->graphicsCmdPool(0), 1, &cmdBuf);
    stagingBuffer.cleanUp();
  }
  // Reference by Jian Ru's Laugh Engine implementation: https://github.com/jian-ru/laugh_engine
  // Research information though https://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf
  //
  for (u32 fi = 0; fi < 6; ++fi ) {
    Vector3 n = GetWorldNormalFromCubeFace(fi);
    u32 ho = fi * height;
    for (u32 py = 0; py < height; ++py) {
      for (u32 px = 0; px < width; ++px) {
        
        Vector3 wi = GetWorldDirection(fi, px, py, width, height);
        r32 dist2 = wi.dot(wi); 
        wi = wi.normalize();
        // Obtain our solid angle differential.
        r32 dw = pixelA * n.dot(-wi) / dist2;
        i32 offset = ho * width + py * width + px * 4;
        Vector3 L = Vector3(static_cast<r32>(data[offset + 0]) / 255.0f,
                            static_cast<r32>(data[offset + 1]) / 255.0f,
                            static_cast<r32>(data[offset + 2]) / 255.0f);

        // Compute our coefficients by taking the sum of each lobe and applying over the distance lighting distribution.
        // which calculates the overall irradiance E(). 
        _shcoeff[0] += L * 0.282095f * dw;
        _shcoeff[1] += L * 0.488603f * dw * wi.y;
        _shcoeff[2] += L * 0.488603f * dw * wi.z;
        _shcoeff[3] += L * 0.488603f * dw * wi.x;
        _shcoeff[4] += L * 1.092548f * dw * wi.x * wi.y;
        _shcoeff[5] += L * 1.092548f * dw * wi.y * wi.z;
        _shcoeff[6] += L * 0.315392f * dw * (3.0f * (wi.z * wi.z) - 1.0f);
        _shcoeff[7] += L * 1.092548f * dw * wi.x * wi.z;
        _shcoeff[8] += L * 0.546274f * dw * (wi.x * wi.x - wi.y * wi.y); 
      }
    }
  }
  
  delete[] data;
}


b32 LightProbe::saveToFile(const std::string& filename)
{
  // TODO(): 
  return false;
}


b32 LightProbe::loadFromFile(const std::string& filename)
{
  memset(_shcoeff, 0, sizeof(_shcoeff));
  _position = Vector3();
  _bias = 1.0f;

  // Load up coefficients here.

  // TODO():
  return false;
}


GlobalIllumination::GlobalIllumination()
  : m_pGlobalIllumination(nullptr)
  , m_pBrdfLUTs(nullptr)
  , m_pEnvMaps(nullptr)
  , m_pGlobalEnvMap(nullptr)
  , m_pGlobalBRDFLUT(nullptr)
  , m_localReflectionsEnabled(false)
  , m_pLocalGIBuffer(nullptr)
  , m_pGlobalGIBuffer(nullptr)
{
}


GlobalIllumination::~GlobalIllumination()
{
}


void GlobalIllumination::initialize(VulkanRHI* pRhi, b32 enableLocalReflections)
{
  m_pGlobalIllumination = pRhi->createDescriptorSet();
  DescriptorSetLayout* layout = nullptr;
  if (enableLocalReflections) {
    layout = globalIllumination_DescLR;
    if (!m_pLocalGIBuffer) {
      m_pLocalGIBuffer = pRhi->createBuffer();
      VkBufferCreateInfo buffCi = { };
      buffCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffCi.size = VkDeviceSize(sizeof(LocalInfoGI));
      buffCi.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      buffCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      m_pLocalGIBuffer->initialize(buffCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
      m_pLocalGIBuffer->Map();
    }
  }
  else {
    layout = globalIllumination_DescNoLR;
  }

  if (!m_pGlobalGIBuffer) {
    m_pGlobalGIBuffer = pRhi->createBuffer();
    VkBufferCreateInfo gBuffCi = {};
    gBuffCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    gBuffCi.size = VkDeviceSize(sizeof(DiffuseSH));
    gBuffCi.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    gBuffCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_pGlobalGIBuffer->initialize(gBuffCi, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    m_pGlobalGIBuffer->Map();
  }

  m_pGlobalIllumination->allocate(pRhi->descriptorPool(), layout);
}


void GlobalIllumination::cleanUp(VulkanRHI* pRhi)
{
  if (m_pGlobalIllumination) {
    pRhi->freeDescriptorSet(m_pGlobalIllumination);
    m_pGlobalIllumination = nullptr;
  }

  if (m_pGlobalGIBuffer) {
    pRhi->freeBuffer(m_pGlobalGIBuffer);
    m_pGlobalGIBuffer = nullptr;
  }

  if (m_pLocalGIBuffer) {
    pRhi->freeBuffer(m_pLocalGIBuffer);
    m_pLocalGIBuffer = nullptr;
  }
}


void GlobalIllumination::updateGlobalGI(VulkanRHI* pRhi)
{
  R_ASSERT(m_pGlobalGIBuffer->Mapped(), "Unmapped global GI data.");
  memcpy(m_pGlobalGIBuffer->Mapped(), &m_globalDiffuseSH, sizeof(DiffuseSH));
  VkMappedMemoryRange range = { };
  range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  range.memory = m_pGlobalGIBuffer->Memory();
  range.offset = 0;
  range.size = VK_WHOLE_SIZE;
  pRhi->logicDevice()->FlushMappedMemoryRanges(1, &range);
}


void GlobalIllumination::update(Renderer* pRenderer)
{
  std::array<VkWriteDescriptorSet, 6> writeSets;
  u32 count = 3;
  updateGlobalGI(pRenderer->getRHI());

  if (m_localReflectionsEnabled) {
    count = 6;
  }

  VkDescriptorBufferInfo globalIrrInfo = {};
  VkDescriptorBufferInfo localIrrInfo = {};

  VkDescriptorImageInfo globalEnvMap = {};
  VkDescriptorImageInfo globalBrdfLut = {};
  VkDescriptorImageInfo localEnvMaps = {};
  VkDescriptorImageInfo localBrdfLuts = {};

  globalIrrInfo.buffer = m_pGlobalGIBuffer->NativeBuffer();
  globalIrrInfo.offset = 0;
  globalIrrInfo.range = VkDeviceSize(sizeof(DiffuseSH));

  localIrrInfo.buffer = m_pLocalGIBuffer->NativeBuffer();
  localIrrInfo.offset = 0;
  localIrrInfo.range = VkDeviceSize(sizeof(LocalInfoGI));

  globalEnvMap.sampler = DefaultSampler2DKey->getHandle();
  globalBrdfLut.sampler = DefaultSampler2DKey->getHandle();
  localEnvMaps.sampler = DefaultSampler2DKey->getHandle();
  localBrdfLuts.sampler = DefaultSampler2DKey->getHandle();

  globalEnvMap.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalBrdfLut.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  localEnvMaps.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  localBrdfLuts.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  Texture* pTexture = m_pGlobalEnvMap;
  Texture* pBRDF = DefaultTextureKey;
  if (m_pGlobalBRDFLUT && m_pGlobalBRDFLUT->getView()) { pBRDF = m_pGlobalBRDFLUT; }

  globalEnvMap.imageView = pTexture->getView();
  globalBrdfLut.imageView = pBRDF->getView();
  // TODO(): These are place holders, we don't have data for these yet!
  // Obtain env and irr maps from scene when building!
  localEnvMaps.imageView = DefaultTextureKey->getView();
  localBrdfLuts.imageView = DefaultTextureKey->getView();

  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writeSets[0].dstBinding = 0;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pBufferInfo = &globalIrrInfo;
  writeSets[0].dstSet = nullptr;
  writeSets[0].pImageInfo = nullptr;
  writeSets[0].pTexelBufferView = nullptr;
  writeSets[0].pNext = nullptr;

  writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[1].descriptorCount = 1;
  writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[1].dstBinding = 1;
  writeSets[1].dstArrayElement = 0;
  writeSets[1].pImageInfo = &globalEnvMap;
  writeSets[1].dstSet = nullptr;
  writeSets[1].pBufferInfo = nullptr;
  writeSets[1].pTexelBufferView = nullptr;
  writeSets[1].pNext = nullptr;

  writeSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[2].descriptorCount = 1;
  writeSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[2].dstBinding = 2;
  writeSets[2].dstArrayElement = 0;
  writeSets[2].pImageInfo = &globalBrdfLut;
  writeSets[2].dstSet = nullptr;
  writeSets[2].pBufferInfo = nullptr;
  writeSets[2].pTexelBufferView = nullptr;
  writeSets[2].pNext = nullptr;

  writeSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[3].descriptorCount = 1;
  writeSets[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writeSets[3].dstBinding = 3;
  writeSets[3].dstArrayElement = 0;
  writeSets[3].pBufferInfo = &localIrrInfo;
  writeSets[3].dstSet = nullptr;
  writeSets[3].pImageInfo = nullptr;
  writeSets[3].pTexelBufferView = nullptr;
  writeSets[3].pNext = nullptr;

  writeSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[4].descriptorCount = 1;
  writeSets[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[4].dstBinding = 4;
  writeSets[4].dstArrayElement = 0;
  writeSets[4].pImageInfo = &localEnvMaps;
  writeSets[4].dstSet = nullptr;
  writeSets[4].pBufferInfo = nullptr;
  writeSets[4].pTexelBufferView = nullptr;
  writeSets[4].pNext = nullptr;

  writeSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[5].descriptorCount = 1;
  writeSets[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[5].dstBinding = 2;
  writeSets[5].dstArrayElement = 0;
  writeSets[5].pImageInfo = &localBrdfLuts;
  writeSets[5].dstSet = nullptr;
  writeSets[5].pBufferInfo = nullptr;
  writeSets[5].pTexelBufferView = nullptr;
  writeSets[5].pNext = nullptr;

  m_pGlobalIllumination->update(count, writeSets.data());
}
} // Recluse