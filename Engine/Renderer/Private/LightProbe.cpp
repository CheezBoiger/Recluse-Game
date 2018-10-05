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

namespace Recluse {


Vector3 GetWorldDirection(u32 faceIdx, u32 px, u32 py, u32 width, u32 height)
{
  Vector3 worldDir = Vector3();

  return worldDir;
}


Vector3 GetWorldNormalFromCubeFace(u32 idx)
{
  // Get cube face normal.
  Vector3 n = Vector3(0.0f);
  n[idx >> 1] = (idx & 1) ? 1.0f : -1.0f;
  return n;
}


void LightProbe::GenerateSHCoefficients(VulkanRHI* rhi, TextureCube* envMap)
{
  u32 width = envMap->WidthPerFace();
  u32 height = envMap->HeightPerFace();
  r32 pixelA = (1.0f / static_cast<r32>(width)) * (1.0f / static_cast<r32>(height));
  u8* data = new u8[width * height * 4];

  {
    VkCommandBuffer cmdBuf;
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = rhi->GraphicsCmdPool(0);
    allocInfo.commandBufferCount = 1;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(rhi->LogicDevice()->Native(), &allocInfo, &cmdBuf);
    
    // Read image data through here.
    // TODO(): 

    vkFreeCommandBuffers(rhi->LogicDevice()->Native(), rhi->GraphicsCmdPool(0), 1, &cmdBuf);
  }
  // Reference by Jian Ru's Laugh Engine implementation: https://github.com/jian-ru/laugh_engine
  // Research information though https://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf
  //
  for (u32 fi = 0; fi < 6; ++fi ) {
    Vector3 n = GetWorldNormalFromCubeFace(fi);
    for (u32 py = 0; py < height; ++py) {
      for (u32 px = 0; px < width; ++px) {
        
        Vector3 wi = GetWorldDirection(fi, px, py, width, height);
        r32 dist2 = wi.Dot(wi); 
        wi = wi.Normalize();
        // Obtain our solid angle differential.
        r32 dw = pixelA * n.Dot(-wi) / dist2;
        Vector3 L = Vector3(data[py * width + px]);

        // Compute our coefficients by taking the sum of each lobe and applying over the distance lighting distribution.
        // which calculates the overall irradiance E(). 
        _shcoeff[0] += L * 0.282095f * dw;
        _shcoeff[1] += L * 0.488603f * dw * wi.y;
        _shcoeff[2] += L * 0.488603f * dw * wi.z;
        _shcoeff[3] += L * 0.488603f * dw * wi.x;
        _shcoeff[4] += L * 1.092548f * dw * wi.x * wi.y;
        _shcoeff[5] += L * 1.092548f * dw * wi.y * wi.z;
        _shcoeff[6] += L * 1.092548f * dw * wi.x * wi.z;
        _shcoeff[7] += L * 0.315392f * dw * (3.0f * (wi.z * wi.z) - 1.0f);
        _shcoeff[8] += L * 0.546274f * dw * (wi.x * wi.x - wi.y * wi.y); 
      }
    }
  }
  
  delete[] data;
}


b32 LightProbe::SaveToFile(const std::string& filename)
{
  // TODO(): 
  return false;
}


b32 LightProbe::LoadFromFile(const std::string& filename)
{
  memset(_shcoeff, 0, sizeof(_shcoeff));
  _position = Vector3();
  _r = 0.0f;

  // Load up coefficients here.

  // TODO():
  return false;
}


GlobalIllumination::GlobalIllumination()
  : m_pGlobalIllumination(nullptr)
  , m_pBrdfLUTs(nullptr)
  , m_pEnvMaps(nullptr)
  , m_pGlobalEnvMap(nullptr)
  , m_pIrrMaps(nullptr)
  , m_localReflectionsEnabled(false)
{
}


GlobalIllumination::~GlobalIllumination()
{
}


void GlobalIllumination::Initialize(VulkanRHI* pRhi, b32 enableLocalReflections)
{
  m_pGlobalIllumination = pRhi->CreateDescriptorSet();
  DescriptorSetLayout* layout = nullptr;
  if (enableLocalReflections) {
    layout = globalIllumination_DescLR;
  }
  else {
    layout = globalIllumination_DescNoLR;
  }

  m_pGlobalIllumination->Allocate(pRhi->DescriptorPool(), layout);
}


void GlobalIllumination::CleanUp(VulkanRHI* pRhi)
{
  if (m_pGlobalIllumination) {
    pRhi->FreeDescriptorSet(m_pGlobalIllumination);
    m_pGlobalIllumination = nullptr;
  }
}


void GlobalIllumination::Update(Renderer* pRenderer)
{
  std::array<VkWriteDescriptorSet, 6> writeSets;
  u32 count = 3;

  if (m_localReflectionsEnabled) {
    count = 6;
  }

  VkDescriptorImageInfo globalIrrMap = {};
  VkDescriptorImageInfo globalEnvMap = {};
  VkDescriptorImageInfo globalBrdfLut = {};
  VkDescriptorImageInfo localIrrMaps = {};
  VkDescriptorImageInfo localEnvMaps = {};
  VkDescriptorImageInfo localBrdfLuts = {};

  globalIrrMap.sampler = DefaultSampler2DKey->Handle();
  globalEnvMap.sampler = DefaultSampler2DKey->Handle();
  globalBrdfLut.sampler = DefaultSampler2DKey->Handle();
  localIrrMaps.sampler = DefaultSampler2DKey->Handle();
  localEnvMaps.sampler = DefaultSampler2DKey->Handle();
  localBrdfLuts.sampler = DefaultSampler2DKey->Handle();

  globalIrrMap.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalEnvMap.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalBrdfLut.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  localIrrMaps.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  localEnvMaps.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  localBrdfLuts.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  Texture* pTexture = m_pGlobalEnvMap;
  globalIrrMap.imageView = pTexture->View();
  globalEnvMap.imageView = pTexture->View();
  globalBrdfLut.imageView = DefaultTextureKey->View();
  // TODO(): These are place holders, we don't have data for these yet!
  // Obtain env and irr maps from scene when building!
  localIrrMaps.imageView = DefaultTextureKey->View();
  localEnvMaps.imageView = DefaultTextureKey->View();
  localBrdfLuts.imageView = DefaultTextureKey->View();

  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[0].dstBinding = 0;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pImageInfo = &globalIrrMap;
  writeSets[0].dstSet = nullptr;
  writeSets[0].pBufferInfo = nullptr;
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
  writeSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[3].dstBinding = 3;
  writeSets[3].dstArrayElement = 0;
  writeSets[3].pImageInfo = &localIrrMaps;
  writeSets[3].dstSet = nullptr;
  writeSets[3].pBufferInfo = nullptr;
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

  m_pGlobalIllumination->Update(count, writeSets.data());
}
} // Recluse