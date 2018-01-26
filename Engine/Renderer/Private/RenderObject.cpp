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
#include "MaterialDescriptor.hpp"
#include "TextureType.hpp"

#include <array>


namespace Recluse {


RenderObject::RenderObject(MeshDescriptor* mesh, MaterialDescriptor* material)
  : _pMaterialDescId(material)
  , _pMeshDescId(mesh)
  , mCurrIdx(0)
  , Instances(1)
  , Renderable(true)
{
  mMeshSets[0] = nullptr;
  mMeshSets[1] = nullptr;
  mMaterialSets[0] = nullptr;
  mMaterialSets[1] = nullptr;
  m_BonesSets[0] = nullptr;
  m_BonesSets[1] = nullptr;
}


RenderObject::~RenderObject()
{
  if (mMeshSets[0] || mMeshSets[1] || mMaterialSets[0] || mMaterialSets[1] || m_BonesSets[0] || m_BonesSets[1]) {
    Log(rError) << "Descriptor Sets were not cleaned up before destruction of this object!\n";
  }
}


void RenderObject::Initialize()
{
  if (mMeshSets[0] || mMeshSets[1] || mMaterialSets[0] || mMaterialSets[1]) {
    R_DEBUG(rNotify, "This RenderObject is already initialized. Skipping...\n");
    return;
  }
  
  // Now create the set to update to.
  mMeshSets[0] = mRhi->CreateDescriptorSet();
  mMeshSets[1] = mRhi->CreateDescriptorSet();
  mMaterialSets[0] = mRhi->CreateDescriptorSet();
  mMaterialSets[1] = mRhi->CreateDescriptorSet();

  DescriptorSetLayout* MeshLayout = gResources().GetDescriptorSetLayout(MeshSetLayoutStr);
  DescriptorSetLayout* MaterialLayout = gResources().GetDescriptorSetLayout(MaterialSetLayoutStr);
  mMeshSets[0]->Allocate(mRhi->DescriptorPool(), MeshLayout);
  mMeshSets[1]->Allocate(mRhi->DescriptorPool(), MeshLayout);
  mMaterialSets[0]->Allocate(mRhi->DescriptorPool(), MaterialLayout);
  mMaterialSets[1]->Allocate(mRhi->DescriptorPool(), MaterialLayout);

  if (_pMeshDescId->Skinned()) {
    DescriptorSetLayout* BonesLayout = gResources().GetDescriptorSetLayout(BonesSetLayoutStr);
    m_BonesSets[0] = mRhi->CreateDescriptorSet();
    m_BonesSets[1] = mRhi->CreateDescriptorSet();
    m_BonesSets[0]->Allocate(mRhi->DescriptorPool(), BonesLayout);
    m_BonesSets[1]->Allocate(mRhi->DescriptorPool(), BonesLayout);
  }

  UpdateDescriptorSets(0);
  UpdateDescriptorSets(1);
}


void RenderObject::CleanUp()
{
  for (size_t idx = 0; idx < 2; ++idx) {
    if (mMeshSets[idx]) {
      mRhi->FreeDescriptorSet(mMeshSets[idx]);
      mMeshSets[idx] = nullptr;
    }
  }

  for (size_t idx = 0; idx < 2; ++idx) {
    if (mMaterialSets[idx]) {
      mRhi->FreeDescriptorSet(mMaterialSets[idx]);
      mMaterialSets[idx] = nullptr;
    }
  }

  for (size_t idx = 0; idx < 2; ++idx) {
    mRhi->FreeDescriptorSet(m_BonesSets[idx]);
    m_BonesSets[idx] = nullptr;
  }
}


void RenderObject::Update()
{
  UpdateDescriptorSets(((mCurrIdx == 0) ? 1 : 0));
}


void RenderObject::UpdateDescriptorSets(size_t idx)
{
  if (!_pMeshDescId || !_pMaterialDescId) {
    Log(rError) << "A mesh descriptor AND material need to be set for this render object in order to update!\n";
    R_ASSERT(false, "No material or mesh descriptor set for this render object, can not update!");
    return;
  }

  if (!_pMaterialDescId->Native()) {
    Log(rError) << "MaterialDescriptor buffer was not initialized prior to initializing this RenderObject! Halting update\n";
    R_ASSERT(false, "MaterialDescriptor Buffer was not intialized, can not continue this RenderObject update.\n");
    return;
  }

  Sampler* sampler = gResources().GetSampler(DefaultSamplerStr);
  if (_pMaterialDescId->Sampler()) sampler = _pMaterialDescId->Sampler()->Handle();

  Texture* defaultTexture = gResources().GetRenderTexture(DefaultTextureStr);

  // Mesh
  {
    VkDescriptorBufferInfo objBufferInfo = {};
    objBufferInfo.buffer = _pMeshDescId->NativeObjectBuffer()->NativeBuffer();
    objBufferInfo.offset = 0;
    objBufferInfo.range = sizeof(ObjectBuffer);
    VkWriteDescriptorSet MeshWriteSet = { };
    MeshWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MeshWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    MeshWriteSet.dstBinding = 0;
    MeshWriteSet.dstArrayElement = 0;
    MeshWriteSet.pBufferInfo = &objBufferInfo;
    MeshWriteSet.descriptorCount = 1;
    MeshWriteSet.pNext = nullptr;
    mMeshSets[idx]->Update(1, &MeshWriteSet);
  }

  {
    if (!_pMaterialDescId->Native()) { 
      Log(rWarning) << "Can not update render object material descriptor sets, material descriptor buffer is not initialized!\n";
      return; 
    }

    std::array<VkWriteDescriptorSet, 7> MaterialWriteSets;
    VkDescriptorImageInfo albedoInfo = {};
    albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (_pMaterialDescId->Albedo()) {
      albedoInfo.imageView = _pMaterialDescId->Albedo()->Handle()->View();
    }
    else {
      albedoInfo.imageView = defaultTexture->View();
    }
    albedoInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo metallicInfo = {};
    metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (_pMaterialDescId->Metallic()) {
      metallicInfo.imageView = _pMaterialDescId->Metallic()->Handle()->View();
    }
    else {
      metallicInfo.imageView = defaultTexture->View();
    }
    metallicInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo roughInfo = {};
    roughInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (_pMaterialDescId->Roughness()) {
      roughInfo.imageView = _pMaterialDescId->Roughness()->Handle()->View();
    }
    else {
      roughInfo.imageView = defaultTexture->View();
    }
    roughInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo normalInfo = {};
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (_pMaterialDescId->Normal()) {
      normalInfo.imageView = _pMaterialDescId->Normal()->Handle()->View();
    }
    else {
      normalInfo.imageView = defaultTexture->View();
    }
    normalInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo aoInfo = {};
    aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (_pMaterialDescId->Ao()) {
      aoInfo.imageView = _pMaterialDescId->Ao()->Handle()->View();
    }
    else {
      aoInfo.imageView = defaultTexture->View();
    }
    aoInfo.sampler = sampler->Handle();

    VkDescriptorImageInfo emissiveInfo = {};
    emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (_pMaterialDescId->Emissive()) {
      emissiveInfo.imageView = _pMaterialDescId->Emissive()->Handle()->View();
    }
    else {
      emissiveInfo.imageView = defaultTexture->View();
    }
    emissiveInfo.sampler = sampler->Handle();

    VkDescriptorBufferInfo matBufferInfo = { };
    matBufferInfo.buffer = _pMaterialDescId->Native()->NativeBuffer();
    matBufferInfo.offset = 0;
    matBufferInfo.range = sizeof(MaterialBuffer);

    MaterialWriteSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[0].dstBinding = 1;
    MaterialWriteSets[0].descriptorCount = 1;
    MaterialWriteSets[0].dstArrayElement = 0;
    MaterialWriteSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[0].pImageInfo = &albedoInfo;
    MaterialWriteSets[0].pNext = nullptr;

    MaterialWriteSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[1].dstBinding = 2;
    MaterialWriteSets[1].descriptorCount = 1;
    MaterialWriteSets[1].dstArrayElement = 0;
    MaterialWriteSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[1].pImageInfo = &metallicInfo;
    MaterialWriteSets[1].pNext = nullptr;

    MaterialWriteSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[2].dstBinding = 3;
    MaterialWriteSets[2].descriptorCount = 1;
    MaterialWriteSets[2].dstArrayElement = 0;
    MaterialWriteSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[2].pImageInfo = &roughInfo;
    MaterialWriteSets[2].pNext = nullptr;

    MaterialWriteSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[3].dstBinding = 4;
    MaterialWriteSets[3].descriptorCount = 1;
    MaterialWriteSets[3].dstArrayElement = 0;
    MaterialWriteSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[3].pImageInfo = &normalInfo;
    MaterialWriteSets[3].pNext = nullptr;

    MaterialWriteSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[4].dstBinding = 5;
    MaterialWriteSets[4].descriptorCount = 1;
    MaterialWriteSets[4].dstArrayElement = 0;
    MaterialWriteSets[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[4].pImageInfo = &aoInfo;
    MaterialWriteSets[4].pNext = nullptr;

    MaterialWriteSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[5].dstBinding = 6;
    MaterialWriteSets[5].descriptorCount = 1;
    MaterialWriteSets[5].dstArrayElement = 0;
    MaterialWriteSets[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    MaterialWriteSets[5].pImageInfo = &emissiveInfo;
    MaterialWriteSets[5].pNext = nullptr;

    MaterialWriteSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MaterialWriteSets[6].dstBinding = 0;
    MaterialWriteSets[6].descriptorCount = 1;
    MaterialWriteSets[6].dstArrayElement = 0;
    MaterialWriteSets[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    MaterialWriteSets[6].pBufferInfo = &matBufferInfo;
    MaterialWriteSets[6].pNext = nullptr;

    mMaterialSets[idx]->Update(static_cast<u32>(MaterialWriteSets.size()), MaterialWriteSets.data());
  }

  // Bones
  if (_pMeshDescId->Skinned()) {
    VkDescriptorBufferInfo boneBufferInfo = { };
    SkinnedMeshDescriptor* sm = reinterpret_cast<SkinnedMeshDescriptor*>(_pMeshDescId);
    boneBufferInfo.buffer = sm->NativeBoneBuffer()->NativeBuffer();
    boneBufferInfo.offset = 0;
    boneBufferInfo.range = sizeof(BonesBuffer);
    VkWriteDescriptorSet BoneWriteSet = { };
    BoneWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    BoneWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    BoneWriteSet.dstBinding = 0;
    BoneWriteSet.pBufferInfo = &boneBufferInfo;
    BoneWriteSet.dstArrayElement = 0;
    BoneWriteSet.descriptorCount = 1;
    BoneWriteSet.pNext = nullptr;

    m_BonesSets[idx]->Update(1, &BoneWriteSet);
  }
}
} // Recluse