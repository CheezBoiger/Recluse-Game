// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Material.hpp"
#include "Resources.hpp"

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
}


void Material::Initialize(b8 isStatic)
{
  Sampler* sampler = gResources().GetSampler("DefaultSampler");
  if (mSampler) sampler = mSampler;

  Texture* defaultTexture = gResources().GetRenderTexture("DefaultTexture");
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
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout("PBRObjectMaterialLayout");
  mObjectBufferSet->Allocate(mRhi->DescriptorPool(), pbrLayout);

  VkBufferCreateInfo bonesCI = { };
  VkDeviceSize bonesSize = sizeof(BonesBuffer);
  bonesCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bonesCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bonesCI.size = bonesSize;    

  mBonesBuffer = mRhi->CreateBuffer();
  mBonesBuffer->Initialize(bonesCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  std::array<VkWriteDescriptorSet, 8> writeSets;

  size_t count = 0;
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
    albedoInfo.imageView = mAlbedo->View();
  }
  else {
    albedoInfo.imageView = defaultTexture->View();
  }
  albedoInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo metallicInfo = {};
  metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mMetallic) {
    metallicInfo.imageView = mMetallic->View();
  }
  else {
    metallicInfo.imageView = defaultTexture->View();
  }
  metallicInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo roughInfo = {};
  roughInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mRoughness) {
    roughInfo.imageView = mRoughness->View();
  }
  else {
    roughInfo.imageView = defaultTexture->View();
  }
  roughInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo normalInfo = {};
  normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mNormal) {
    normalInfo.imageView = mNormal->View();
  }
  else {
    normalInfo.imageView = defaultTexture->View();
  }
  normalInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo aoInfo = {};
  aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mAo) {
    aoInfo.imageView = mAo->View();
  }
  else {
    aoInfo.imageView = defaultTexture->View();
  }
  aoInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo emissiveInfo = {};
  emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (mEmissive) {
    emissiveInfo.imageView = mEmissive->View();
  }
  else {
    emissiveInfo.imageView = defaultTexture->View();
  }
  emissiveInfo.sampler = sampler->Handle();

  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[count].dstBinding = 0;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].pBufferInfo = &objBufferInfo;
  writeSets[count].descriptorCount = 1;
  writeSets[count].pNext = nullptr;
  count++;

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

  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[count].dstBinding = 1;
  writeSets[count].pBufferInfo = &boneBufferInfo;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].descriptorCount = 1;
  writeSets[count].pNext = nullptr;
  count++;

  if (!mObjectBufferSet) {
    R_DEBUG("ERROR: Cannot update uninitialized descriptor set in material! Material is either uninitialized or cleaned up!\n");
    return;
  }

  mObjectBufferSet->Update(static_cast<u32>(count), writeSets.data());
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


void GlobalMaterial::Initialize()
{
  if (!mRhi) {
    R_DEBUG("ERROR: No RHI owner set in this Global Material upon initialization!\n");
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
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout("PBRGlobalMaterialLayout");

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


void LightMaterial::Initialize()
{
  // TODO
  if (!mRhi) {
    R_DEBUG("ERROR: RHI owner not set for light material upon initialization!\n");
    return;
  }

  mLightBuffer = mRhi->CreateBuffer();
  VkBufferCreateInfo bufferCI = { };
  VkDeviceSize dSize = sizeof(LightBuffer);
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  
  mLightBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout("PBRLightMaterialLayout");
  mDescriptorSet = mRhi->CreateDescriptorSet();
  mDescriptorSet->Allocate(mRhi->DescriptorPool(), pbrLayout);

  VkDescriptorBufferInfo lightBufferInfo = {};
  lightBufferInfo.buffer = mLightBuffer->Handle();
  lightBufferInfo.offset = 0;
  lightBufferInfo.range = sizeof(LightBuffer);

  std::array<VkWriteDescriptorSet, 1> writeSets;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pBufferInfo = &lightBufferInfo;
  writeSets[0].pNext = nullptr;
  writeSets[0].dstBinding = 0;

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