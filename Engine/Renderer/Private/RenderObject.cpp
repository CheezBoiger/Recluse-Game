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
#include "MeshDescriptor.hpp"
#include "MeshData.hpp"
#include "Material.hpp"
#include "TextureType.hpp"

#include <array>


namespace Recluse {


RenderObject::RenderObject(MeshDescriptor* mesh, Material* material)
  : MaterialId(material)
  , MeshDescriptorId(mesh)
  , Skinned(false)
  , mCurrIdx(0)
  , Instances(1)
  , Renderable(true)
{
  mDescriptorSets[0] = nullptr;
  mDescriptorSets[1] = nullptr;
}


RenderObject::~RenderObject()
{
  if (mDescriptorSets[0] || mDescriptorSets[1]) {
    Log(rError) << "Descriptor Set was not cleaned up before destruction of this object!\n";
  }
}


void RenderObject::Initialize()
{
  if (!MeshDescriptorId || !MaterialId) {
    Log(rError) << "A mesh descriptor AND material need to be set for this render object!\n"; 
    R_ASSERT(false, "No material or mesh descriptor set for this render object!");
    return;
  } 

  if (mDescriptorSets[0] || mDescriptorSets[1]) {
    R_DEBUG(rNotify, "This RenderObject is already initialized. Skipping...\n");
    return;
  }
  
  // Now create the set to update to.
  mDescriptorSets[0] = mRhi->CreateDescriptorSet();
  mDescriptorSets[1] = mRhi->CreateDescriptorSet();

  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout(PBRObjMatLayoutStr);
  mDescriptorSets[0]->Allocate(mRhi->DescriptorPool(), pbrLayout);
  mDescriptorSets[1]->Allocate(mRhi->DescriptorPool(), pbrLayout);

  UpdateDescriptorSet(0, true);
  UpdateDescriptorSet(1, true);
}


void RenderObject::CleanUp()
{
  if (mDescriptorSets[0]) {
    mRhi->FreeDescriptorSet(mDescriptorSets[0]);
    mDescriptorSets[0] = nullptr;
  }

  if (mDescriptorSets[1]) {
    mRhi->FreeDescriptorSet(mDescriptorSets[1]);
    mDescriptorSets[1] = nullptr;
  }
}


void RenderObject::Update()
{
  UpdateDescriptorSet(((mCurrIdx == 0) ? 1 : 0), false);
}


void RenderObject::UpdateDescriptorSet(size_t idx, b8 includeBufferUpdate)
{
  std::array<VkWriteDescriptorSet, 8> writeSets;
  size_t count = 0;

  Sampler* sampler = gResources().GetSampler(DefaultSamplerStr);
  if (MaterialId->Sampler()) sampler = MaterialId->Sampler()->Handle();

  Texture* defaultTexture = gResources().GetRenderTexture(DefaultTextureStr);

  VkDescriptorBufferInfo objBufferInfo = {};
  objBufferInfo.buffer = MeshDescriptorId->NativeObjectBuffer()->NativeBuffer();
  objBufferInfo.offset = 0;
  objBufferInfo.range = sizeof(ObjectBuffer);

  VkDescriptorBufferInfo boneBufferInfo = {};
  if (Skinned) {
    SkinnedMeshDescriptor* sm = reinterpret_cast<SkinnedMeshDescriptor*>(MeshDescriptorId);
    boneBufferInfo.buffer = sm->NativeBoneBuffer()->NativeBuffer();
    boneBufferInfo.offset = 0;
    boneBufferInfo.range = sizeof(BonesBuffer);
  }

  VkDescriptorImageInfo albedoInfo = {};
  albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (MaterialId->Albedo()) {
    albedoInfo.imageView = MaterialId->Albedo()->Handle()->View();
  }
  else {
    albedoInfo.imageView = defaultTexture->View();
  }
  albedoInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo metallicInfo = {};
  metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (MaterialId->Metallic()) {
    metallicInfo.imageView = MaterialId->Metallic()->Handle()->View();
  }
  else {
    metallicInfo.imageView = defaultTexture->View();
  }
  metallicInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo roughInfo = {};
  roughInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (MaterialId->Roughness()) {
    roughInfo.imageView = MaterialId->Roughness()->Handle()->View();
  }
  else {
    roughInfo.imageView = defaultTexture->View();
  }
  roughInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo normalInfo = {};
  normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (MaterialId->Normal()) {
    normalInfo.imageView = MaterialId->Normal()->Handle()->View();
  }
  else {
    normalInfo.imageView = defaultTexture->View();
  }
  normalInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo aoInfo = {};
  aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (MaterialId->Ao()) {
    aoInfo.imageView = MaterialId->Ao()->Handle()->View();
  }
  else {
    aoInfo.imageView = defaultTexture->View();
  }
  aoInfo.sampler = sampler->Handle();

  VkDescriptorImageInfo emissiveInfo = {};
  emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if (MaterialId->Emissive()) {
    emissiveInfo.imageView = MaterialId->Emissive()->Handle()->View();
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

    if (Skinned) {
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

  if (!mDescriptorSets[idx]) {
    R_DEBUG(rError, "Cannot update uninitialized descriptor set in RenderObject! Descriptor is either null or cleaned up!\n");
    return;
  }

  mDescriptorSets[idx]->Update(static_cast<u32>(count), writeSets.data());
}
} // Recluse