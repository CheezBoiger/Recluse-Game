// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshDescriptor.hpp"
#include "MeshData.hpp"
#include "Renderer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "Core/Exception.hpp"


namespace Recluse {

MeshDescriptor::MeshDescriptor()
  : m_Visible(true)
  , m_Renderable(true)
  , m_Translucent(false)
  , m_Static(true) 
  , m_Skinned(false)
  , m_pRhi(nullptr)
  , m_bNeedsUpdate(true)
{
  m_ObjectData._HasBones = false;
  m_ObjectData._LoD = 0.0f;
}


MeshDescriptor::~MeshDescriptor()
{
  if (m_pObjectBuffer) {
    R_DEBUG(rWarning, "Object buffer from mesh was not properly cleaned up!\n");
  }
}


void MeshDescriptor::Initialize()
{
  // Create the render buffer for the object.
  m_pObjectBuffer = m_pRhi->CreateBuffer();
  VkDeviceSize objectSize = sizeof(ObjectBuffer);
  VkBufferCreateInfo objectCI = {};
  objectCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  objectCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  objectCI.size = objectSize;
  objectCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  m_pObjectBuffer->Initialize(objectCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}


void MeshDescriptor::Update()
{
  if (m_bNeedsUpdate) {
    m_pObjectBuffer->Map();
      memcpy(m_pObjectBuffer->Mapped(), &m_ObjectData, sizeof(ObjectBuffer));
    m_pObjectBuffer->UnMap();
    m_bNeedsUpdate = false;
  }
}


void MeshDescriptor::CleanUp()
{

  if (m_pObjectBuffer) {
    m_pRhi->FreeBuffer(m_pObjectBuffer);
    m_pObjectBuffer = nullptr;
  }
}


SkinnedMeshDescriptor::SkinnedMeshDescriptor()
  : m_pBonesBuffer(nullptr)
  , MeshDescriptor()
{
  m_Skinned = true;
}


SkinnedMeshDescriptor::~SkinnedMeshDescriptor()
{
  if (m_pBonesBuffer) {
    R_DEBUG(rWarning, "Skinned mesh bones buffer was not cleaned up before destroying!\n");
  }
}


void SkinnedMeshDescriptor::Initialize()
{
  MeshDescriptor::Initialize();

  VkBufferCreateInfo bonesCI = {};
  VkDeviceSize bonesSize = sizeof(BonesBuffer);
  bonesCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bonesCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bonesCI.size = bonesSize;

  m_pBonesBuffer = m_pRhi->CreateBuffer();
  m_pBonesBuffer->Initialize(bonesCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}


void SkinnedMeshDescriptor::Update()
{
  MeshDescriptor::Update();

  m_pBonesBuffer->Map();
    memcpy(m_pBonesBuffer->Mapped(), &m_BonesData, sizeof(BonesBuffer));
  m_pBonesBuffer->UnMap();
}


void SkinnedMeshDescriptor::CleanUp()
{
  MeshDescriptor::CleanUp();

  if (m_pBonesBuffer) {
    m_pRhi->FreeBuffer(m_pBonesBuffer);
    m_pBonesBuffer = nullptr;
  }
}
} // Recluse