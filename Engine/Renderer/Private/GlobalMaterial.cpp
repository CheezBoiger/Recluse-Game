// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GlobalMaterial.hpp"
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


GlobalMaterial::GlobalMaterial()
  : mGlobalBuffer(nullptr)
  , mDescriptorSet(nullptr)
{
  mGlobal.screenSize[0] = 0;
  mGlobal.screenSize[1] = 0;
  mGlobal.gamma = 2.2f;
  mGlobal.bloomEnabled = true;
  mGlobal.exposure = 1.0f;
  mGlobal.pad1 = 0;
}


GlobalMaterial::~GlobalMaterial()
{
  if (mGlobalBuffer) {
    R_DEBUG(rWarning, "Global buffer was not cleaned up!\n");
  }

  if (mDescriptorSet) {
    R_DEBUG(rWarning, "Global material was not properly cleaned up!\n");
  }
}


void GlobalMaterial::Initialize()
{
  if (!mRhi) {
    R_DEBUG(rError, "No RHI owner set in this Global Material upon initialization!\n");
    return;
  }

  if (mGlobalBuffer || mDescriptorSet) {
    R_DEBUG(rNotify, "This global buffer is already intialized! Skipping...\n");
    return;
  }

  mGlobalBuffer = mRhi->CreateBuffer();
  VkDeviceSize dSize = sizeof(GlobalBuffer);
  VkBufferCreateInfo bufferCI = {};
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  mGlobalBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout(PBRGlobalMatLayoutStr);

  mDescriptorSet = mRhi->CreateDescriptorSet();
  mDescriptorSet->Allocate(mRhi->DescriptorPool(), pbrLayout);

  VkDescriptorBufferInfo globalBufferInfo = {};
  globalBufferInfo.buffer = mGlobalBuffer->NativeBuffer();
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


void GlobalMaterial::Update()
{
  mGlobalBuffer->Map();
  memcpy(mGlobalBuffer->Mapped(), &mGlobal, sizeof(GlobalBuffer));
  mGlobalBuffer->UnMap();
}
} // Recluse