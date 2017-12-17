// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshDescriptor.hpp"
#include "MeshData.hpp"
#include "Renderer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {

MeshDescriptor::MeshDescriptor()
  : mVisible(true)
  , mRenderable(true)
  , mTranslucent(false)
  , mStatic(true) 
  , mSkinned(false)
{
  mObjectData.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  mObjectData.lodBias = 0.0f;
  mObjectData.transparency = 1.0f;
  mObjectData.hasAlbedo = false;
  mObjectData.hasAO = false;
  mObjectData.hasBones = false;
  mObjectData.hasEmissive = false;
  mObjectData.hasMetallic = false;
  mObjectData.hasNormal = false;
  mObjectData.hasRoughness = false;
  mObjectData.isTransparent = false;
  mObjectData.baseMetal = 0.0f;
  mObjectData.baseRough = 1.0f;
}


MeshDescriptor::~MeshDescriptor()
{
  if (mObjectBuffer) {
    R_DEBUG(rWarning, "Object buffer from mesh was not properly cleaned up!\n");
  }
}


void MeshDescriptor::Initialize(Renderer* renderer)
{
  mRenderer = renderer;

  // Create the render buffer for the object.
  mObjectBuffer = mRenderer->RHI()->CreateBuffer();
  VkDeviceSize objectSize = sizeof(ObjectBuffer);
  VkBufferCreateInfo objectCI = {};
  objectCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  objectCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  objectCI.size = objectSize;
  objectCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  mObjectBuffer->Initialize(objectCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}


void MeshDescriptor::Update()
{
  mObjectBuffer->Map();
    memcpy(mObjectBuffer->Mapped(), &mObjectData, sizeof(ObjectBuffer));
  mObjectBuffer->UnMap();
}


void MeshDescriptor::CleanUp()
{

  if (mObjectBuffer) {
    mRenderer->RHI()->FreeBuffer(mObjectBuffer);
    mObjectBuffer = nullptr;
  }
}


SkinnedMeshDescriptor::SkinnedMeshDescriptor()
  : mBonesBuffer(nullptr)
  , MeshDescriptor()
{
  mSkinned = true;
}


SkinnedMeshDescriptor::~SkinnedMeshDescriptor()
{
  if (mBonesBuffer) {
    R_DEBUG(rWarning, "Skinned mesh bones buffer was not cleaned up before destroying!\n");
  }
}


void SkinnedMeshDescriptor::Initialize(Renderer* renderer)
{
  MeshDescriptor::Initialize(renderer);

  VkBufferCreateInfo bonesCI = {};
  VkDeviceSize bonesSize = sizeof(BonesBuffer);
  bonesCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bonesCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bonesCI.size = bonesSize;

  mBonesBuffer = mRenderer->RHI()->CreateBuffer();
  mBonesBuffer->Initialize(bonesCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}


void SkinnedMeshDescriptor::Update()
{
  MeshDescriptor::Update();

  mBonesBuffer->Map();
  memcpy(mBonesBuffer->Mapped(), &mBonesData, sizeof(BonesBuffer));
  mBonesBuffer->UnMap();
}


void SkinnedMeshDescriptor::CleanUp()
{
  MeshDescriptor::CleanUp();

  if (mBonesBuffer) {
    mRenderer->RHI()->FreeBuffer(mBonesBuffer);
    mBonesBuffer = nullptr;
  }
}
} // Recluse