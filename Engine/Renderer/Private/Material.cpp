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
  , mObjectBuffer(nullptr)
  , mBonesBuffer(nullptr)
  , mSampler(nullptr)
{ 
  mObjectData.lodBias = 0.0f;
  mObjectData.hasAlbedo = false;
  mObjectData.hasAO = false;
  mObjectData.hasBones = false;
  mObjectData.hasEmissive = false;
  mObjectData.hasMetallic = false;
  mObjectData.hasNormal = false;
  mObjectData.hasRoughness = false;
}


void Material::Initialize(b8 isStatic)
{
  Sampler* sampler = gResources().GetSampler(DefaultSamplerStr);
  if (mSampler) sampler = mSampler->Handle();

  Texture* defaultTexture = gResources().GetRenderTexture(DefaultTextureStr);
  // Create the render buffer for the object.
  mObjectBuffer = mRhi->CreateBuffer();
  VkDeviceSize objectSize = sizeof(ObjectBuffer);
  VkBufferCreateInfo objectCI = { };
  objectCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  objectCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  objectCI.size = objectSize;
  objectCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  mObjectBuffer->Initialize(objectCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Now create the set to update to.
  mObjectBufferSet = mRhi->CreateDescriptorSet();
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout(PBRObjMatLayoutStr);
  mObjectBufferSet->Allocate(mRhi->DescriptorPool(), pbrLayout);

  VkBufferCreateInfo bonesCI = { };
  VkDeviceSize bonesSize = sizeof(BonesBuffer);
  bonesCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bonesCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bonesCI.size = bonesSize;    

  mBonesBuffer = mRhi->CreateBuffer();
  mBonesBuffer->Initialize(bonesCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  UpdateDescriptorSet(true);
}


void Material::CleanUp()
{
  if (mObjectBuffer) {
    mRhi->FreeBuffer(mObjectBuffer);
    mObjectBuffer = nullptr;
  }

  if (mObjectBufferSet) {
    mRhi->FreeDescriptorSet(mObjectBufferSet);
    mObjectBufferSet = nullptr;
  }

  if (mBonesBuffer) {
    mRhi->FreeBuffer(mBonesBuffer);
    mBonesBuffer = nullptr;
  }
}


void Material::Update() 
{
  mObjectBuffer->Map();
    memcpy(mObjectBuffer->Mapped(), &mObjectData, sizeof(ObjectBuffer));
  mObjectBuffer->UnMap();
}


void Material::UpdateDescriptorSet(b8 includeBufferUpdate)
{
  std::array<VkWriteDescriptorSet, 8> writeSets;
  size_t count = 0;

  Sampler* sampler = gResources().GetSampler(DefaultSamplerStr);
  if (mSampler) sampler = mSampler->Handle();

  Texture* defaultTexture = gResources().GetRenderTexture(DefaultTextureStr);

  VkDescriptorBufferInfo objBufferInfo = {};
  objBufferInfo.buffer = mObjectBuffer->Handle();
  objBufferInfo.offset = 0;
  objBufferInfo.range = sizeof(ObjectBuffer);

  VkDescriptorBufferInfo boneBufferInfo = {};
  if (mBonesBuffer) {
    boneBufferInfo.buffer = mBonesBuffer->Handle();
    boneBufferInfo.offset = 0;
    boneBufferInfo.range = sizeof(BonesBuffer);
  }

  VkDescriptorImageInfo albedoInfo = {};
  albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mAlbedo) {
    albedoInfo.imageView = mAlbedo->Handle()->View();
  }
  else {
    albedoInfo.imageView = defaultTexture->View();
  }
  albedoInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo metallicInfo = {};
  metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mMetallic) {
    metallicInfo.imageView = mMetallic->Handle()->View();
  }
  else {
    metallicInfo.imageView = defaultTexture->View();
  }
  metallicInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo roughInfo = {};
  roughInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mRoughness) {
    roughInfo.imageView = mRoughness->Handle()->View();
  }
  else {
    roughInfo.imageView = defaultTexture->View();
  }
  roughInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo normalInfo = {};
  normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mNormal) {
    normalInfo.imageView = mNormal->Handle()->View();
  }
  else {
    normalInfo.imageView = defaultTexture->View();
  }
  normalInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo aoInfo = {};
  aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mAo) {
    aoInfo.imageView = mAo->Handle()->View();
  }
  else {
    aoInfo.imageView = defaultTexture->View();
  }
  aoInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo emissiveInfo = {};
  emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mEmissive) {
    emissiveInfo.imageView = mEmissive->Handle()->View();
  }
  else {
    emissiveInfo.imageView = defaultTexture->View();
  }
  emissiveInfo.sampler = sampler->Handle();


  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].dstBinding = 2;
  writeSets[count].descriptorCount = 1;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[count].pImageInfo = &albedoInfo;
  writeSets[count].pNext = nullptr;
  count++;

  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].dstBinding = 3;
  writeSets[count].descriptorCount = 1;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[count].pImageInfo = &metallicInfo;
  writeSets[count].pNext = nullptr;
  count++;

  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].dstBinding = 4;
  writeSets[count].descriptorCount = 1;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[count].pImageInfo = &roughInfo;
  writeSets[count].pNext = nullptr;
  count++;


  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].dstBinding = 5;
  writeSets[count].descriptorCount = 1;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[count].pImageInfo = &normalInfo;
  writeSets[count].pNext = nullptr;
  count++;

  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].dstBinding = 6;
  writeSets[count].descriptorCount = 1;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[count].pImageInfo = &aoInfo;
  writeSets[count].pNext = nullptr;
  count++;

  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].dstBinding = 7;
  writeSets[count].descriptorCount = 1;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[count].pImageInfo = &emissiveInfo;
  writeSets[count].pNext = nullptr;
  count++;

  // Include uniform buffer update if these have changed internally, which 
  // is not really going to happen.
  if (includeBufferUpdate) {
    writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeSets[count].dstBinding = 0;
    writeSets[count].dstArrayElement = 0;
    writeSets[count].pBufferInfo = &objBufferInfo;
    writeSets[count].descriptorCount = 1;
    writeSets[count].pNext = nullptr;
    count++;

    writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeSets[count].dstBinding = 1;
    writeSets[count].pBufferInfo = &boneBufferInfo;
    writeSets[count].dstArrayElement = 0;
    writeSets[count].descriptorCount = 1;
    writeSets[count].pNext = nullptr;
    count++;
  }

  if (!mObjectBufferSet) {
    R_DEBUG(rError, "Cannot update uninitialized descriptor set in material! Material is either uninitialized or cleaned up!\n");
    return;
  }

  mObjectBufferSet->Update(static_cast<u32>(count), writeSets.data());
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