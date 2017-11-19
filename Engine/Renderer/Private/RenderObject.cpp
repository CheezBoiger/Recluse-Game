// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RenderObject.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"

#include "Resources.hpp"
#include "RendererData.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "TextureType.hpp"

#include <array>

namespace Recluse {


RenderObject::RenderObject(Mesh* mesh, Material* material)
  : materialId(material)
  , meshId(mesh)
  , skinned(false)
{
}


RenderObject::~RenderObject()
{
  if (mDescriptorSet) {
    Log(rError) << "Descriptor Set was not cleaned up before destruction of this object!\n";
  }
}


void RenderObject::Initialize()
{
  if (!meshId || !materialId) {
    Log(rError) << "A mesh AND material need to be set for this render object!\n"; 
    R_ASSERT(false, "No material or mesh set for this render object!");
    return;
  } 
  // Now create the set to update to.
  mDescriptorSet = mRhi->CreateDescriptorSet();
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout(PBRObjMatLayoutStr);
  mDescriptorSet->Allocate(mRhi->DescriptorPool(), pbrLayout);
  UpdateDescriptorSet(true);
}


void RenderObject::CleanUp()
{
  if (mDescriptorSet) {
    mRhi->FreeDescriptorSet(mDescriptorSet);
    mDescriptorSet = nullptr;
  }
}


void RenderObject::Update()
{
  UpdateDescriptorSet(false);
}


void RenderObject::UpdateDescriptorSet(b8 includeBufferUpdate)
{
  std::array<VkWriteDescriptorSet, 8> writeSets;
  size_t count = 0;

  Sampler* sampler = gResources().GetSampler(DefaultSamplerStr);
  if (materialId->Sampler()) sampler = materialId->Sampler()->Handle();

  Texture* defaultTexture = gResources().GetRenderTexture(DefaultTextureStr);

  VkDescriptorBufferInfo objBufferInfo = {};
  objBufferInfo.buffer = meshId->NativeObjectBuffer()->Handle();
  objBufferInfo.offset = 0;
  objBufferInfo.range = sizeof(ObjectBuffer);

  VkDescriptorBufferInfo boneBufferInfo = {};
  if (skinned) {
    SkinnedMesh* sm = reinterpret_cast<SkinnedMesh*>(meshId);
    boneBufferInfo.buffer = sm->NativeBoneBuffer()->Handle();
    boneBufferInfo.offset = 0;
    boneBufferInfo.range = sizeof(BonesBuffer);
  }

  VkDescriptorImageInfo albedoInfo = {};
  albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (materialId->Albedo()) {
    albedoInfo.imageView = materialId->Albedo()->Handle()->View();
  }
  else {
    albedoInfo.imageView = defaultTexture->View();
  }
  albedoInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo metallicInfo = {};
  metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (materialId->Metallic()) {
    metallicInfo.imageView = materialId->Metallic()->Handle()->View();
  }
  else {
    metallicInfo.imageView = defaultTexture->View();
  }
  metallicInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo roughInfo = {};
  roughInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (materialId->Roughness()) {
    roughInfo.imageView = materialId->Roughness()->Handle()->View();
  }
  else {
    roughInfo.imageView = defaultTexture->View();
  }
  roughInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo normalInfo = {};
  normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (materialId->Normal()) {
    normalInfo.imageView = materialId->Normal()->Handle()->View();
  }
  else {
    normalInfo.imageView = defaultTexture->View();
  }
  normalInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo aoInfo = {};
  aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (materialId->Ao()) {
    aoInfo.imageView = materialId->Ao()->Handle()->View();
  }
  else {
    aoInfo.imageView = defaultTexture->View();
  }
  aoInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo emissiveInfo = {};
  emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (materialId->Emissive()) {
    emissiveInfo.imageView = materialId->Emissive()->Handle()->View();
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

    if (skinned) {
      writeSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeSets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeSets[count].dstBinding = 1;
      writeSets[count].pBufferInfo = &boneBufferInfo;
      writeSets[count].dstArrayElement = 0;
      writeSets[count].descriptorCount = 1;
      writeSets[count].pNext = nullptr;
      count++;
    }
  }

  if (!mDescriptorSet) {
    R_DEBUG(rError, "Cannot update uninitialized descriptor set in material! Material is either uninitialized or cleaned up!\n");
    return;
  }

  mDescriptorSet->Update(static_cast<u32>(count), writeSets.data());
}
} // Recluse