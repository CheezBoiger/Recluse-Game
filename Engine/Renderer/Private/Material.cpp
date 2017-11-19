// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Material.hpp"
#include "Resources.hpp"
#include "RendererData.hpp"
#include "TextureType.hpp"

#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"
#include "RHI/VulkanRHI.hpp"

#include "Core/Exception.hpp"

#include <array>


namespace Recluse {


Material::Material()
  : mAlbedo(nullptr)
  , mMetallic(nullptr)
  , mRoughness(nullptr)
  , mNormal(nullptr)
  , mAo(nullptr)
  , mEmissive(nullptr)
  , mSampler(nullptr)
{ 
}


GlobalMaterial::GlobalMaterial()
{
  mGlobal.screenSize[0] = 0;
  mGlobal.screenSize[1] = 0;
  mGlobal.pad[0] = 0;
  mGlobal.pad[1] = 0;
}


void GlobalMaterial::Initialize()
{
  if (!mRhi) {
    R_DEBUG(rError, "No RHI owner set in this Global Material upon initialization!\n");
    return;
  }

  mGlobalBuffer = mRhi->CreateBuffer();
  VkDeviceSize dSize = sizeof(GlobalBuffer);
  VkBufferCreateInfo bufferCI = { };
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  mGlobalBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout(PBRGlobalMatLayoutStr);

  mDescriptorSet = mRhi->CreateDescriptorSet();
  mDescriptorSet->Allocate(mRhi->DescriptorPool(), pbrLayout);

  VkDescriptorBufferInfo globalBufferInfo = {};
  globalBufferInfo.buffer = mGlobalBuffer->Handle();
  globalBufferInfo.offset = 0;
  globalBufferInfo.range = sizeof(GlobalBuffer);

  std::array<VkWriteDescriptorSet, 1> writeSets;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstBinding = 0;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].pNext = nullptr;
  writeSets[0].pBufferInfo = &globalBufferInfo;

  mDescriptorSet->Update(static_cast<u32>(writeSets.size()), writeSets.data());
}


LightMaterial::LightMaterial()
  : mShadowMap(nullptr)
  , mShadowSampler(nullptr)
  , mRhi(nullptr)
  , mDescriptorSet(nullptr)
  , mLightBuffer(nullptr)
{
  mLights.primaryLight.enable = false;
  //mLights.primaryLight.pad[0] = 0;
  //mLights.primaryLight.pad[1] = 0;
  //mLights.primaryLight.pad[2] = 0;
  for (size_t i = 0; i < 128; ++i) {
    mLights.pointLights[i].position = Vector4();
    mLights.pointLights[i].color = Vector4();
    mLights.pointLights[i].range = 0.0f;
    mLights.pointLights[i].enable = false;
    //mLights.pointLights[i].pad[0] = 0.0f;
    //mLights.pointLights[i].pad[1] = 0.0f;
    //mLights.pointLights[i].pad[2] = 0.0f;
  }
}


void LightMaterial::Initialize()
{
  // TODO
  if (!mRhi) {
    R_DEBUG(rError, "RHI owner not set for light material upon initialization!\n");
    return;
  }

  // This class has already been initialized.
  if (mLightBuffer || mDescriptorSet)  return; 

  mLightBuffer = mRhi->CreateBuffer();
  VkBufferCreateInfo bufferCI = { };
  VkDeviceSize dSize = sizeof(LightBuffer);
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  mLightBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  
  // Create our shadow map texture.
  if (!mShadowMap) {
    mShadowMap = mRhi->CreateTexture();

    VkImageCreateInfo imageCi = { };
    imageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  }

  if (!mShadowSampler) {
    mShadowSampler = mRhi->CreateSampler();
  }

  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout(PBRLightMatLayoutStr);
  mDescriptorSet = mRhi->CreateDescriptorSet();
  mDescriptorSet->Allocate(mRhi->DescriptorPool(), pbrLayout);

  VkDescriptorBufferInfo lightBufferInfo = {};
  lightBufferInfo.buffer = mLightBuffer->Handle();
  lightBufferInfo.offset = 0;
  lightBufferInfo.range = sizeof(LightBuffer);

  // TODO(): Once we create our shadow map, we will add it here.
  VkDescriptorImageInfo globalShadowInfo = {};
  globalShadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  globalShadowInfo.imageView = gResources().GetRenderTexture(DefaultTextureStr)->View();
  globalShadowInfo.sampler = gResources().GetSampler(DefaultSamplerStr)->Handle();

  std::array<VkWriteDescriptorSet, 2> writeSets;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pBufferInfo = &lightBufferInfo;
  writeSets[0].pNext = nullptr;
  writeSets[0].dstBinding = 0;

  writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[1].descriptorCount = 1;
  writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[1].dstArrayElement = 0;
  writeSets[1].pNext = nullptr;
  writeSets[1].pImageInfo = &globalShadowInfo;
  writeSets[1].dstBinding = 1;

  mDescriptorSet->Update(static_cast<u32>(writeSets.size()), writeSets.data());
}


void GlobalMaterial::CleanUp()
{
  // TODO
  if (mDescriptorSet) {
    mRhi->FreeDescriptorSet(mDescriptorSet);
    mDescriptorSet = nullptr;
  }

  if (mGlobalBuffer) {
    mRhi->FreeBuffer(mGlobalBuffer);
    mGlobalBuffer = nullptr;
  }
}


void LightMaterial::CleanUp()
{
  if (mShadowMap) {
    mRhi->FreeTexture(mShadowMap);
    mShadowMap = nullptr;
  }

  if (mShadowSampler) {
    mRhi->FreeSampler(mShadowSampler);
    mShadowSampler = nullptr;
  }

  // TODO
  if (mDescriptorSet) {
    mRhi->FreeDescriptorSet(mDescriptorSet);
    mDescriptorSet = nullptr;
  }

  if (mLightBuffer) {
    mRhi->FreeBuffer(mLightBuffer);
    mLightBuffer = nullptr;
  }
}


void GlobalMaterial::Update()
{
  mGlobalBuffer->Map();
    memcpy(mGlobalBuffer->Mapped(), &mGlobal, sizeof(GlobalBuffer));
  mGlobalBuffer->UnMap();
}


void LightMaterial::Update()
{
  mLightBuffer->Map();
    memcpy(mLightBuffer->Mapped(), &mLights, sizeof(LightBuffer));
  mLightBuffer->UnMap();
}
} // Recluse