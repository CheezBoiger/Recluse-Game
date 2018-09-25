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
  , m_bNeedsUpdate(true)
{
  m_ObjectData._HasJoints = false;
  m_ObjectData._LoD = 0.0f;
  m_ObjectData._w0 = 0.0f;
  m_ObjectData._w1 = 0.0f;
  m_meshSet = nullptr;

}


MeshDescriptor::~MeshDescriptor()
{
  if (m_meshSet) {
    Log(rError) << "Mesh Descriptor Set were not cleaned up before destruction of this object!\n";
  }
  if (m_pObjectBuffer) {
    R_DEBUG(rWarning, "Object buffer from mesh was not properly cleaned up!\n");
  }
}


void MeshDescriptor::Initialize(VulkanRHI* pRhi)
{
  // Create the render buffer for the object.
  m_pObjectBuffer = pRhi->CreateBuffer();
  VkDeviceSize objectSize = sizeof(ObjectBuffer);
  VkBufferCreateInfo objectCI = {};
  objectCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  objectCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  objectCI.size = objectSize;
  objectCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  m_pObjectBuffer->Initialize(objectCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_pObjectBuffer->Map();
  m_meshSet = pRhi->CreateDescriptorSet();

  DescriptorSetLayout* MeshLayout = MeshSetLayoutKey;
  m_meshSet->Allocate(pRhi->DescriptorPool(), MeshLayout);
}


void MeshDescriptor::Update(VulkanRHI* pRhi)
{
  // Mesh
  if ((m_bNeedsUpdate & MESH_DESCRIPTOR_UPDATE_BIT)) {
    VkDescriptorBufferInfo objBufferInfo = {};
    objBufferInfo.buffer = m_pObjectBuffer->NativeBuffer();
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
    m_meshSet->Update(1, &MeshWriteSet);
    //SwapDescriptorSet();
  }

  if ((m_bNeedsUpdate & MESH_BUFFER_UPDATE_BIT)) {
    R_ASSERT(m_pObjectBuffer->Mapped(), "Object buffer was not mapped.");
    memcpy(m_pObjectBuffer->Mapped(), &m_ObjectData, sizeof(ObjectBuffer));

    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pObjectBuffer->Memory();
    range.size = m_pObjectBuffer->MemorySize();
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  m_bNeedsUpdate = 0;
}


void MeshDescriptor::CleanUp(VulkanRHI* pRhi)
{
  // Need to wait before we can remove this object in cmd buffer.
  pRhi->GraphicsWaitIdle(DEFAULT_QUEUE_IDX);
  if (m_meshSet) {
    pRhi->FreeDescriptorSet(m_meshSet);
    m_meshSet = nullptr;
  }

  if (m_pObjectBuffer) {
    m_pObjectBuffer->UnMap();
    pRhi->FreeBuffer(m_pObjectBuffer);
    m_pObjectBuffer = nullptr;
  }
}


JointDescriptor::JointDescriptor()
  : m_pJointsBuffer(nullptr)
  , m_jointSet(nullptr)
  , m_bNeedsUpdate(true)
{
}


JointDescriptor::~JointDescriptor()
{
  if (m_pJointsBuffer) {
    R_DEBUG(rWarning, "Skinned mesh bones buffer was not cleaned up before destroying!\n");
  }
}


void JointDescriptor::Initialize(VulkanRHI* pRhi)
{
  VkBufferCreateInfo jointCI = {};
  VkDeviceSize jointsSize = sizeof(JointBuffer);
  jointCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  jointCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  jointCI.size = jointsSize;

  m_pJointsBuffer = pRhi->CreateBuffer();
  m_pJointsBuffer->Initialize(jointCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  m_pJointsBuffer->Map();

  DescriptorSetLayout* jointLayout = BonesSetLayoutKey;

  m_jointSet = pRhi->CreateDescriptorSet();

  m_jointSet->Allocate(pRhi->DescriptorPool(), jointLayout);

  UpdateJointSets();
}


void JointDescriptor::Update(VulkanRHI* pRhi)
{
  if ((m_bNeedsUpdate & JOINT_DESCRIPTOR_UPDATE_BIT)) {
    R_DEBUG(rDebug, "Updating Joint Sets.\n");
    UpdateJointSets();
  }

  if ((m_bNeedsUpdate & JOINT_BUFFER_UPDATE_BIT)) {
    R_ASSERT(m_pJointsBuffer->Mapped(), "Joint buffer was not mapped.!");
    memcpy(m_pJointsBuffer->Mapped(), &m_jointsData, sizeof(JointBuffer));

    VkMappedMemoryRange range = { };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_pJointsBuffer->Memory();
    range.size = m_pJointsBuffer->MemorySize();
    pRhi->LogicDevice()->FlushMappedMemoryRanges(1, &range);
  }

  m_bNeedsUpdate = 0;
}


void JointDescriptor::CleanUp(VulkanRHI* pRhi)
{
  pRhi->FreeDescriptorSet(m_jointSet);
  m_jointSet = nullptr;

  if (m_pJointsBuffer) {
    pRhi->FreeBuffer(m_pJointsBuffer);
    m_pJointsBuffer = nullptr;
  }
}


void JointDescriptor::UpdateJointSets()
{
  // Bones
  R_DEBUG(rNotify, "Updating bone descriptor set.\n");
  VkDescriptorBufferInfo boneBufferInfo = {};
  boneBufferInfo.buffer = m_pJointsBuffer->NativeBuffer();
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

  m_jointSet->Update(1, &BoneWriteSet);
}
} // Recluse