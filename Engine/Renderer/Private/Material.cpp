// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Material.hpp"

#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"

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


void Material::Update() 
{
  std::array<VkWriteDescriptorSet, 8> writeSets;

  VkDescriptorBufferInfo objBufferInfo = { };
  objBufferInfo.buffer = mObjectBuffer->Handle();
  objBufferInfo.offset = 0;
  objBufferInfo.range = sizeof(ObjectBuffer);

  VkDescriptorBufferInfo boneBufferInfo = { };
  boneBufferInfo.buffer = mBonesBuffer->Handle();
  boneBufferInfo.offset = 0;
  boneBufferInfo.range = sizeof(BonesBuffer);

  VkDescriptorImageInfo albedoInfo = { };
  albedoInfo.imageView = mAlbedo->View();
  albedoInfo.sampler = mSampler->Handle();
  albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkDescriptorImageInfo metallicInfo = { };
  metallicInfo.sampler = mSampler->Handle();
  metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  metallicInfo.imageView = mMetallic->View();

  VkDescriptorImageInfo roughInfo = { };
  roughInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  roughInfo.sampler = mSampler->Handle();
  roughInfo.imageView = mRoughness->View();

  VkDescriptorImageInfo normalInfo = { };
  normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  normalInfo.sampler = mSampler->Handle();
  normalInfo.imageView = mNormal->View();
  
  VkDescriptorImageInfo aoInfo = { };
  aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  aoInfo.imageView = mAo->View();
  aoInfo.sampler = mSampler->Handle();

  VkDescriptorImageInfo emissiveInfo = { };
  emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  emissiveInfo.imageView = mEmissive->View();
  emissiveInfo.sampler = mSampler->Handle();

  writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[0].dstBinding = 0;
  writeSets[0].dstArrayElement = 0;
  writeSets[0].pBufferInfo = &objBufferInfo;
  writeSets[0].descriptorCount = 1;

  writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeSets[1].dstBinding = 1;
  writeSets[1].pBufferInfo = &boneBufferInfo;
  writeSets[1].dstArrayElement = 0;
  writeSets[1].descriptorCount = 1;

  writeSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[2].dstBinding = 2;
  writeSets[2].descriptorCount = 1;
  writeSets[2].dstArrayElement = 0;
  writeSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[2].pImageInfo = &albedoInfo;

  writeSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[3].dstBinding = 3;
  writeSets[3].descriptorCount = 1;
  writeSets[3].dstArrayElement = 0;
  writeSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[3].pImageInfo = &metallicInfo;

  writeSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[4].dstBinding = 4;
  writeSets[4].descriptorCount = 1;
  writeSets[4].dstArrayElement = 0;
  writeSets[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[4].pImageInfo = &roughInfo;

  writeSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[5].dstBinding = 5;
  writeSets[5].descriptorCount = 1;
  writeSets[5].dstArrayElement = 0;
  writeSets[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[5].pImageInfo = &normalInfo;

  writeSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[6].dstBinding = 6;
  writeSets[6].descriptorCount = 1;
  writeSets[6].dstArrayElement = 0;
  writeSets[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[6].pImageInfo = &aoInfo;

  writeSets[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeSets[7].dstBinding = 7;
  writeSets[7].descriptorCount = 1;
  writeSets[7].dstArrayElement = 0;
  writeSets[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeSets[7].pImageInfo = &emissiveInfo;

  mObjectBufferSetRef->Update(u32(writeSets.size()), writeSets.data());
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