// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Mesh.hpp"
#include "MeshData.hpp"
#include "Renderer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {

Mesh::Mesh()
  : mVisible(true)
  , mRenderable(true)
  , mTransparent(false)
  , mTranslucent(false)
  , mStatic(true) 
  , mMeshData(nullptr)
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


void Mesh::Initialize(Renderer* renderer, MeshData* data)
{
  mRenderer = renderer;
  mMeshData = data;

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


void Mesh::Update()
{
  mObjectBuffer->Map();
    memcpy(mObjectBuffer->Mapped(), &mObjectData, sizeof(ObjectBuffer));
  mObjectBuffer->UnMap();
}


void Mesh::CleanUp()
{

  if (mObjectBuffer) {
    mRenderer->RHI()->FreeBuffer(mObjectBuffer);
    mObjectBuffer = nullptr;
  }
}


SkinnedMesh::SkinnedMesh()
  : mBonesBuffer(nullptr)
{
}


void SkinnedMesh::Initialize(Renderer* renderer, MeshData* data)
{
  Mesh::Initialize(renderer, data);

  VkBufferCreateInfo bonesCI = {};
  VkDeviceSize bonesSize = sizeof(BonesBuffer);
  bonesCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bonesCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bonesCI.size = bonesSize;

  mBonesBuffer = mRenderer->RHI()->CreateBuffer();
  mBonesBuffer->Initialize(bonesCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}


void SkinnedMesh::Update()
{
  Mesh::Update();

  mBonesBuffer->Map();
  memcpy(mBonesBuffer->Mapped(), &mBonesData, sizeof(BonesBuffer));
  mBonesBuffer->UnMap();
}


void SkinnedMesh::CleanUp()
{
  Mesh::CleanUp();

  if (mBonesBuffer) {
    mRenderer->RHI()->FreeBuffer(mBonesBuffer);
    mBonesBuffer = nullptr;
  }
}
} // Recluse