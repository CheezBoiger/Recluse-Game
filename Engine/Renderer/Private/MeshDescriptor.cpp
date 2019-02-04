// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshDescriptor.hpp"
#include "MeshData.hpp"
#include "Renderer.hpp"
#include "RendererData.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/Buffer.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"


namespace Recluse {


Matrix4 JointBuffer::defaultMatrices[64] = { 
  Matrix4(), Matrix4(), Matrix4(), Matrix4(), 
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4(),
  Matrix4(), Matrix4(), Matrix4(), Matrix4()
};

MeshDescriptor::MeshDescriptor()
  : m_Visible(true)
  , m_Static(true) 
{
  m_ObjectData._HasJoints = false;
  m_ObjectData._LoD = 0.0f;
  m_ObjectData._w0 = 0.0f;
  m_ObjectData._w1 = 0.0f;
}


MeshDescriptor::~MeshDescriptor()
{
  for (u32 i = 0; i < m_pGpuHandles.size(); ++i) {
    if (m_pGpuHandles[i]._pSet) {
      Log(rError) << "Mesh Descriptor Set were not cleaned up before destruction of this object!\n";
    }
    if (m_pGpuHandles[i]._pBuf) {
      R_DEBUG(rWarning, "Object buffer from mesh was not properly cleaned up!\n");
    }
  }
}


void MeshDescriptor::Initialize(VulkanRHI* pRhi)
{
  VkDeviceSize objectSize = sizeof(ObjectBuffer);
  VkBufferCreateInfo objectCI = {};
  objectCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  objectCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  objectCI.size = objectSize;
  objectCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  DescriptorSetLayout* MeshLayout = MeshSetLayoutKey;

  m_pGpuHandles.resize(pRhi->BufferingCount());

  // Create the render buffer for the object.
  for (u32 i = 0; i < m_pGpuHandles.size(); ++i) {
    m_pGpuHandles[i]._pBuf = pRhi->CreateBuffer();
    m_pGpuHandles[i]._pBuf->Initialize(objectCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    m_pGpuHandles[i]._pBuf->Map();
    m_pGpuHandles[i]._pSet = pRhi->CreateDescriptorSet();
    m_pGpuHandles[i]._pSet->Allocate(pRhi->DescriptorPool(), MeshLayout);
    m_pGpuHandles[i]._updates = MESH_BUFFER_UPDATE_BIT | MESH_DESCRIPTOR_UPDATE_BIT;
  }
}


void MeshDescriptor::Update(VulkanRHI* pRhi, u32 frameIndex)
{
  // Mesh
  Buffer* pBuf = m_pGpuHandles[frameIndex]._pBuf;
  DescriptorSet* pSet = m_pGpuHandles[frameIndex]._pSet;
  u32& updates = m_pGpuHandles[frameIndex]._updates;

  if ((updates & MESH_DESCRIPTOR_UPDATE_BIT)) {
    VkDescriptorBufferInfo objBufferInfo = {};
    objBufferInfo.buffer = pBuf->NativeBuffer();
    objBufferInfo.offset = 0;
    objBufferInfo.range = sizeof(ObjectBuffer);
    VkWriteDescriptorSet MeshWriteSet = {};
    MeshWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    MeshWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    MeshWriteSet.dstBinding = 0;
    MeshWriteSet.dstArrayElement = 0;
    MeshWriteSet.pBufferInfo = &objBufferInfo;
    MeshWriteSet.descriptorCount = 1;
    MeshWriteSet.pNext = nullptr;
    pSet->Update(1, &MeshWriteSet);
    //SwapDescriptorSet();
  }

  if ((updates & MESH_BUFFER_UPDATE_BIT)) {
    R_ASSERT(pBuf->Mapped(), "Object buffer was not mapped.");
    memcpy(pBuf->Mapped(), &m_ObjectData, sizeof(ObjectBuffer));

    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = pBuf->Memory();
    range.size = VK_WHOLE_SIZE;
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  // configure back to 0, since we have updated this buffer.
  updates = 0;
}


void MeshDescriptor::CleanUp(VulkanRHI* pRhi)
{
  // Need to wait before we can remove this object in cmd buffer.
  pRhi->GraphicsWaitIdle(DEFAULT_QUEUE_IDX);
  for (u32 i = 0; i < m_pGpuHandles.size(); ++i) {
    if (m_pGpuHandles[i]._pSet) {
      pRhi->FreeDescriptorSet(m_pGpuHandles[i]._pSet);
      m_pGpuHandles[i]._pSet = nullptr;
    }

    if (m_pGpuHandles[i]._pBuf) {
      m_pGpuHandles[i]._pBuf->UnMap();
      pRhi->FreeBuffer(m_pGpuHandles[i]._pBuf);
      m_pGpuHandles[i]._pBuf = nullptr;
    }
  }
}


JointDescriptor::JointDescriptor()
{
}


JointDescriptor::~JointDescriptor()
{
  DEBUG_OP(
  for (u32 i = 0; i < m_pJointHandles.size(); ++i) {
    if (m_pJointHandles[i]._pBuf) {
      R_DEBUG(rWarning, "Skinned mesh bones buffer was not cleaned up before destroying!\n");
    }
  }
  );
}


void JointDescriptor::Initialize(VulkanRHI* pRhi)
{
  VkBufferCreateInfo jointCI = {};
  VkDeviceSize jointsSize = sizeof(JointBuffer);
  jointCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  jointCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  jointCI.size = jointsSize;

  m_pJointHandles.resize(pRhi->BufferingCount());

  for (u32 i = 0; i < m_pJointHandles.size(); ++i) {
    m_pJointHandles[i]._pBuf = pRhi->CreateBuffer();
    m_pJointHandles[i]._pBuf->Initialize(jointCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    m_pJointHandles[i]._pBuf->Map();

    DescriptorSetLayout* jointLayout = BonesSetLayoutKey;

    m_pJointHandles[i]._pSet = pRhi->CreateDescriptorSet();
    m_pJointHandles[i]._pSet->Allocate(pRhi->DescriptorPool(), jointLayout);
    UpdateJointSets(i);
  }
}


void JointDescriptor::Update(VulkanRHI* pRhi, u32 frameIndex)
{
  UpdateManager& m = m_pJointHandles[frameIndex];
  u32 updates = m._updates;
  Buffer* pBuf = m._pBuf;

  if ((updates & JOINT_DESCRIPTOR_UPDATE_BIT)) {
    R_DEBUG(rDebug, "Updating Joint Sets.\n");
    UpdateJointSets(frameIndex);
  }

  if ((updates & JOINT_BUFFER_UPDATE_BIT)) {
    R_ASSERT(pBuf->Mapped(), "Joint buffer was not mapped.!");
    memcpy(pBuf->Mapped(), &m_jointsData, sizeof(JointBuffer));

    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = pBuf->Memory();
    range.size = VK_WHOLE_SIZE;
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  m._updates = 0;
}


void JointDescriptor::CleanUp(VulkanRHI* pRhi)
{
  for (u32 i = 0; i < m_pJointHandles.size(); ++i) {
    pRhi->FreeDescriptorSet(m_pJointHandles[i]._pSet);
    m_pJointHandles[i]._pSet = nullptr;

    if (m_pJointHandles[i]._pBuf) {
      pRhi->FreeBuffer(m_pJointHandles[i]._pBuf);
      m_pJointHandles[i]._pBuf = nullptr;
    }
  }
}


void JointDescriptor::UpdateJointSets(u32 frameIndex)
{
  // Bones
  R_DEBUG(rNotify, "Updating bone descriptor set.\n");
  VkDescriptorBufferInfo boneBufferInfo = {};
  boneBufferInfo.buffer = m_pJointHandles[frameIndex]._pBuf->NativeBuffer();
  boneBufferInfo.offset = 0;
  boneBufferInfo.range = sizeof(JointBuffer);
  VkWriteDescriptorSet BoneWriteSet = {};
  BoneWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  BoneWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  BoneWriteSet.dstBinding = 0;
  BoneWriteSet.pBufferInfo = &boneBufferInfo;
  BoneWriteSet.dstArrayElement = 0;
  BoneWriteSet.descriptorCount = 1;
  BoneWriteSet.pNext = nullptr;

  m_pJointHandles[frameIndex]._pSet->Update(1, &BoneWriteSet);
}
} // Recluse