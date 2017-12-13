// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "LightMaterial.hpp"
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

LightMaterial::LightMaterial()
  : mShadowMap(nullptr)
  , mShadowSampler(nullptr)
  , mRhi(nullptr)
  , mDescriptorSet(nullptr)
  , mLightBuffer(nullptr)
{
  mLights.primaryLight.enable = false;
  mLights.primaryLight.pad[0] = 0;
  mLights.primaryLight.pad[1] = 0;
  //mLights.primaryLight.pad[2] = 0;
  for (size_t i = 0; i < 128; ++i) {
    mLights.pointLights[i].position = Vector4();
    mLights.pointLights[i].color = Vector4();
    mLights.pointLights[i].range = 0.0f;
    mLights.pointLights[i].enable = false;
    mLights.pointLights[i].pad[0] = 0;
    mLights.pointLights[i].pad[1] = 0;
    //mLights.pointLights[i].pad[2] = 0.0f;
  }

  for (size_t i = 0; i < 32; ++i) {
    mLights.directionalLights[i].direction = Vector4();
    mLights.directionalLights[i].enable = false;
    mLights.directionalLights[i].intensity = 1.0f;
    mLights.directionalLights[i].color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  }
}


LightMaterial::~LightMaterial()
{
  if (mLightBuffer) {
    R_DEBUG(rWarning, "Light buffer was not cleaned up!\n");
  }

  if (mDescriptorSet) {
    R_DEBUG(rWarning, "Light Material descriptor set was not properly cleaned up!\n");
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
  if (mLightBuffer || mDescriptorSet)  {
    R_DEBUG(rNotify, "This light buffer is already initialized! Skipping...\n");
    return;
  }

  mLightBuffer = mRhi->CreateBuffer();
  VkBufferCreateInfo bufferCI = {};
  VkDeviceSize dSize = sizeof(LightBuffer);
  bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCI.size = dSize;
  bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  mLightBuffer->Initialize(bufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Create our shadow map texture.
  if (!mShadowMap) {
    mShadowMap = mRhi->CreateTexture();

    VkImageCreateInfo imageCi = {};
    imageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  }

  if (!mShadowSampler) {
    mShadowSampler = mRhi->CreateSampler();
  }

  DescriptorSetLayout* pbrLayout = gResources().GetDescriptorSetLayout(PBRLightMatLayoutStr);
  mDescriptorSet = mRhi->CreateDescriptorSet();
  mDescriptorSet->Allocate(mRhi->DescriptorPool(), pbrLayout);

  VkDescriptorBufferInfo lightBufferInfo = {};
  lightBufferInfo.buffer = mLightBuffer->NativeBuffer();
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


void LightMaterial::Update()
{
  mLightBuffer->Map();
  memcpy(mLightBuffer->Mapped(), &mLights, sizeof(LightBuffer));
  mLightBuffer->UnMap();
}
} // Recluse