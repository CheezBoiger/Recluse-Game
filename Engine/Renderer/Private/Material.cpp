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
  // Create the render buffer for the object.
  mObjectBuffer = mRhi->CreateBuffer();
  VkDeviceSize objectSize = sizeof(ObjectBuffer);
  VkBufferCreateInfo objectCI = { };
  objectCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  objectCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  objectCI.size = objectSize;

  mObjectBuffer->Initialize(objectCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Now create the set to update to.
  mObjectBufferSet = mRhi->CreateDescriptorSet();
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout("PBRObjectMaterialLayout");
  mObjectBufferSet->Allocate(mRhi->DescriptorPool(), pbrLayout);

  if (!isStatic) {
    VkBufferCreateInfo bonesCI = { };
    VkDeviceSize bonesSize = sizeof(BonesBuffer);
    bonesCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bonesCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bonesCI.size = bonesSize;    

    mBonesBuffer = mRhi->CreateBuffer();
    mBonesBuffer->Initialize(bonesCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mObjectData.hasBones = true;
  }
}


void Material::CleanUp()
{
  if (mObjectBuffer) {
    mRhi->FreeBuffer(mObjectBuffer);
  }

  if (mObjectBufferSet) {
    mRhi->FreeDescriptorSet(mObjectBufferSet);
  }

  if (mBonesBuffer) {
    mRhi->FreeBuffer(mBonesBuffer);
  }
}


void Material::Update() 
{
  std::array<VkWriteDescriptorSet, 8> writeSets;

  size_t count = 0;
  VkDescriptorBufferInfo objBufferInfo = { };
  objBufferInfo.buffer = mObjectBuffer->Handle();
  objBufferInfo.offset = 0;
  objBufferInfo.range = sizeof(ObjectBuffer);

  VkDescriptorBufferInfo boneBufferInfo = { };
  if (mBonesBuffer) {
    boneBufferInfo.buffer = mBonesBuffer->Handle();
    boneBufferInfo.offset = 0;
    boneBufferInfo.range = sizeof(BonesBuffer);
  }

  VkDescriptorImageInfo albedoInfo = { };
  if (mAlbedo) {
    albedoInfo.imageView = mAlbedo->View();
    albedoInfo.sampler = mSampler->Handle();
    albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  VkDescriptorImageInfo metallicInfo = { };
  if (mMetallic) {
    metallicInfo.sampler = mSampler->Handle();
    metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    metallicInfo.imageView = mMetallic->View();
  }

  VkDescriptorImageInfo roughInfo = { };
  if (mRoughness) {
    roughInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    roughInfo.sampler = mSampler->Handle();
    roughInfo.imageView = mRoughness->View();
  }

  VkDescriptorImageInfo normalInfo = { };
  if (mNormal) {
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalInfo.sampler = mSampler->Handle();
    normalInfo.imageView = mNormal->View();
  }

  VkDescriptorImageInfo aoInfo = { };
  if (mAo) {
    aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    aoInfo.imageView = mAo->View();
    aoInfo.sampler = mSampler->Handle();
  }

  VkDescriptorImageInfo emissiveInfo = { };
  if (mEmissive) {
    emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    emissiveInfo.imageView = mEmissive->View();
    emissiveInfo.sampler = mSampler->Handle();
  }

  writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[count].dstBinding = 0;
  writeSets[count].dstArrayElement = 0;
  writeSets[count].pBufferInfo = &objBufferInfo;
  writeSets[count].descriptorCount = 1;
  writeSets[count].pNext = nullptr;
  count++;

  if (mObjectData.hasAlbedo) {
    writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[count].dstBinding = 2;
    writeSets[count].descriptorCount = 1;
    writeSets[count].dstArrayElement = 0;
    writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSets[count].pImageInfo = &albedoInfo;
    writeSets[count].pNext = nullptr;
    count++;
  }
 
  if (mObjectData.hasMetallic) {
    writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[count].dstBinding = 3;
    writeSets[count].descriptorCount = 1;
    writeSets[count].dstArrayElement = 0;
    writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSets[count].pImageInfo = &metallicInfo;
    writeSets[count].pNext = nullptr;
    count++;
  }
  
  if (mObjectData.hasRoughness) {
    writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[count].dstBinding = 4;
    writeSets[count].descriptorCount = 1;
    writeSets[count].dstArrayElement = 0;
    writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSets[count].pImageInfo = &roughInfo;
    writeSets[count].pNext = nullptr;
    count++;
  }
  
  if (mObjectData.hasNormal) {
    writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[count].dstBinding = 5;
    writeSets[count].descriptorCount = 1;
    writeSets[count].dstArrayElement = 0;
    writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSets[count].pImageInfo = &normalInfo;
    writeSets[count].pNext = nullptr;
    count++;
  }

  if (mObjectData.hasAO) {
    writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[count].dstBinding = 6;
    writeSets[count].descriptorCount = 1;
    writeSets[count].dstArrayElement = 0;
    writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSets[count].pImageInfo = &aoInfo;
    writeSets[count].pNext = nullptr;
    count++;
  }

  if (mObjectData.hasEmissive) {
    writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[count].dstBinding = 7;
    writeSets[count].descriptorCount = 1;
    writeSets[count].dstArrayElement = 0;
    writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSets[count].pImageInfo = &emissiveInfo;
    writeSets[count].pNext = nullptr;
    count++;
  }

  if (mBonesBuffer) {
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
    R_DEBUG("ERROR: Cannot update uninitialized descriptor set in material! Material is either uninitialized or cleaned up!");
    return;
  } 

  mObjectBufferSet->Update(static_cast<u32>(count), writeSets.data());
}


void GlobalMaterial::Initialize()
{
  // TODO
}


void LightMaterial::Initialize()
{
  // TODO
}


void GlobalMaterial::CleanUp()
{
  // TODO
}


void LightMaterial::CleanUp()
{
  // TODO
}


void GlobalMaterial::Update()
{
  VkDescriptorBufferInfo globalBufferInfo = { };
  globalBufferInfo.buffer = mGlobalBuffer->Handle();
  globalBufferInfo.offset = 0;
  globalBufferInfo.range = sizeof(GlobalBuffer);
  

  std::array<VkWriteDescriptorSet, 1> writeSets;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstBinding = 0;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].pBufferInfo = &globalBufferInfo;

  mDescriptorSet->Update(static_cast<u32>(writeSets.size()), writeSets.data());
}


void LightMaterial::Update()
{
  VkDescriptorBufferInfo lightBufferInfo = { };
  lightBufferInfo.buffer = mLightBuffer->Handle();
  lightBufferInfo.offset = 0;
  lightBufferInfo.range = sizeof(LightBuffer);

  std::array<VkWriteDescriptorSet, 1> writeSets;
  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorCount = 1;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pBufferInfo = &lightBufferInfo;
  writeSets[0].dstBinding = 0;

  mDescriptorSet->Update(static_cast<u32>(writeSets.size()), writeSets.data());
}
} // Recluse