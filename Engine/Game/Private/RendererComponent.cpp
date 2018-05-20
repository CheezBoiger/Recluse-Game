// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererComponent.hpp"
#include "MeshComponent.hpp"
#include "MaterialComponent.hpp"
#include "AnimationComponent.hpp"
#include "GameObject.hpp"

#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


DEFINE_COMPONENT_MAP(RendererComponent);
DEFINE_COMPONENT_MAP(SkinnedRendererComponent);


RendererComponent::RendererComponent()
  : m_meshDescriptor(nullptr)
  , m_materialRef(nullptr)
  , m_meshRef(nullptr)
  , m_configs(CMD_RENDERABLE_BIT | CMD_SHADOWS_BIT)
{
}


RendererComponent::RendererComponent(const RendererComponent& m)
  : m_meshDescriptor(m.m_meshDescriptor)
  , m_materialRef(m.m_materialRef)
  , m_meshRef(m.m_meshRef)
{
}


RendererComponent::RendererComponent(RendererComponent&& m)
  : m_meshDescriptor(m.m_meshDescriptor)
  , m_materialRef(m.m_materialRef)
  , m_meshRef(m.m_meshRef)
{
  m.m_meshDescriptor = nullptr;
  m.m_materialRef = nullptr;
  m.m_meshRef = nullptr;
}


RendererComponent& RendererComponent::operator=(RendererComponent&& obj)
{
  m_meshDescriptor = obj.m_meshDescriptor;
  m_materialRef = obj.m_materialRef;
  m_meshRef = obj.m_meshRef;

  obj.m_meshDescriptor = nullptr;
  obj.m_materialRef = nullptr;
  obj.m_meshRef = nullptr;
  return (*this);
}


RendererComponent& RendererComponent::operator=(const RendererComponent& obj)
{
  m_meshDescriptor = obj.m_meshDescriptor;
  m_meshRef = obj.m_meshRef;
  m_materialRef = obj.m_materialRef;
  return (*this);
}


void RendererComponent::EnableShadow(b32 enable)
{
  if (enable) { m_configs |= CMD_SHADOWS_BIT; }
  else { m_configs &= ~CMD_SHADOWS_BIT; }
}


b32 RendererComponent::ShadowEnabled() const
{
  return (m_configs & CMD_SHADOWS_BIT);
}

void RendererComponent::OnInitialize(GameObject* owner)
{
  Material* material = Material::Default();
  if (m_materialRef && m_materialRef->GetMaterial()) {
    material = m_materialRef->GetMaterial();
  }
  
  m_meshDescriptor = gRenderer().CreateStaticMeshDescriptor();
  m_meshDescriptor->Initialize();
  m_meshDescriptor->PushUpdate(MeshDescriptor::MESH_DESCRIPTOR_UPDATE);

  REGISTER_COMPONENT(RendererComponent, this);
}


void RendererComponent::OnCleanUp()
{
  gRenderer().FreeMeshDescriptor(m_meshDescriptor);
  m_meshDescriptor = nullptr;

  UNREGISTER_COMPONENT(RendererComponent);
}


void RendererComponent::OnEnable()
{
  if (Enabled()) { m_configs |= CMD_RENDERABLE_BIT; }
  else { m_configs &= ~CMD_RENDERABLE_BIT; }
}


void RendererComponent::Update()
{
  if (!Enabled() || !m_meshRef) return;
  // TODO(): Static objects don't necessarily need to be updated all the time.
  // This is especially true if the object is kinematic
  Transform* transform = GetOwner()->GetTransform();
  ObjectBuffer* renderData = m_meshDescriptor->ObjectData();
  Matrix4 model = transform->GetLocalToWorldMatrix();

  // Now push the object into the renderer for updating.
  CmdList<MeshRenderCmd>& list = gRenderer().GetDeferredList();
  MeshRenderCmd cmd;
  cmd._pMeshDesc = m_meshDescriptor;
  cmd._pMatDesc = m_materialRef->GetMaterial()->Native();
  cmd._pMeshData = m_meshRef->MeshRef() ? m_meshRef->MeshRef()->Native() : nullptr;
  cmd._config = m_configs;
  list.PushBack(cmd);

  if (model == renderData->_Model) return;
  Matrix4 N = model;
  N[3][0] = 0.0f;
  N[3][1] = 0.0f;
  N[3][2] = 0.0f;
  N[3][3] = 1.0f;
  renderData->_Model = model;
  renderData->_NormalMatrix = N.Inverse().Transpose();
  m_meshDescriptor->PushUpdate(MeshDescriptor::MESH_BUFFER_UPDATE);
}


void SkinnedRendererComponent::Update()
{
  JointBuffer* pJointBuffer = m_meshDescriptor->JointData();
  R_ASSERT(pJointBuffer, "Joint buffer found null!");
  u32 jointCount = m_meshDescriptor->NumJoints();

  // TODO(): Need to extract joint pose matrices from animation
  // component!

  for (u32 i = 0; i < jointCount; ++i) {
    // Update descriptor joints.
    pJointBuffer->_mJoints[i];
  }
  m_meshDescriptor->PushUpdate(MeshDescriptor::JOINT_BUFFER_UPDATE);
  RendererComponent::Update();
}


void SkinnedRendererComponent::OnInitialize(GameObject* owner)
{

  Material* material = Material::Default();
  if (m_materialRef && m_materialRef->GetMaterial()) {
    material = m_materialRef->GetMaterial();
  }

  m_meshDescriptor = gRenderer().CreateSkinnedMeshDescriptor();
  m_meshDescriptor->Initialize();
  m_meshDescriptor->PushUpdate(MeshDescriptor::MESH_DESCRIPTOR_UPDATE | MeshDescriptor::JOINT_DESCRIPTOR_UPDATE);

  REGISTER_COMPONENT(SkinnedRendererComponent, this);
}


void SkinnedRendererComponent::OnCleanUp()
{
  gRenderer().FreeMeshDescriptor(m_meshDescriptor);
  m_meshDescriptor = nullptr;

  UNREGISTER_COMPONENT(SkinnedRendererComponent);
}
} // Recluse