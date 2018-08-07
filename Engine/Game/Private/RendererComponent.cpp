// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererComponent.hpp"
#include "MeshComponent.hpp"
#include "MaterialComponent.hpp"
#include "AnimationComponent.hpp"
#include "GameObject.hpp"
#include "Camera.hpp"

#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Utility/Profile.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


DEFINE_COMPONENT_MAP(RendererComponent);
DEFINE_COMPONENT_MAP(SkinnedRendererComponent);


RendererComponent::RendererComponent()
  : m_meshDescriptor(nullptr)
  , m_configs(CMD_RENDERABLE_BIT | CMD_SHADOWS_BIT)
  , m_bDirty(true)
  , m_currLod(Mesh::kMeshLodZero)
  , m_allowLod(true)
{
}


RendererComponent::RendererComponent(const RendererComponent& m)
  : m_meshDescriptor(m.m_meshDescriptor)
  , m_meshes(m.m_meshes)
{
}


RendererComponent::RendererComponent(RendererComponent&& m)
  : m_meshDescriptor(m.m_meshDescriptor)
  , m_meshes(std::move(m.m_meshes))
{
  m.m_meshDescriptor = nullptr;
}


RendererComponent& RendererComponent::operator=(RendererComponent&& obj)
{
  m_meshDescriptor = obj.m_meshDescriptor;
  m_meshes = std::move(obj.m_meshes);

  obj.m_meshDescriptor = nullptr;
  return (*this);
}


RendererComponent& RendererComponent::operator=(const RendererComponent& obj)
{
  m_meshDescriptor = obj.m_meshDescriptor;
  m_meshes = obj.m_meshes;
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
  m_meshDescriptor = gRenderer().CreateMeshDescriptor();
  m_meshDescriptor->Initialize(gRenderer().RHI());
  m_meshDescriptor->PushUpdate(MESH_DESCRIPTOR_UPDATE_BIT);

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


void RendererComponent::ForceForward(b32 enable)
{
  if (enable) { m_configs |= CMD_FORWARD_BIT; }
  else { m_configs &= ~CMD_FORWARD_BIT; }
}


void RendererComponent::Update()
{
  if (!Enabled() || m_meshes.empty()) return;
  // TODO(): Static objects don't necessarily need to be updated all the time.
  // This is especially true if the object is kinematic
  Transform* transform = GetOwner()->GetTransform();
  ObjectBuffer* renderData = m_meshDescriptor->ObjectData();
  Matrix4 model = transform->GetLocalToWorldMatrix();

  UpdateLod(transform);

  // Now push the object into the renderer for updating.
  for (size_t i = 0; i < m_meshes.size(); ++i) {
    MeshRenderCmd cmd;
    cmd._pMeshDesc = m_meshDescriptor;
    cmd._bSkinned = Skinned();
    cmd._pJointDesc = GetJointDescriptor();
    cmd._config = m_configs;

    // Push mesh data to renderer.
    Mesh* pMesh = m_meshes[i];
    MeshData* data = pMesh->GetMeshDataLod(m_currLod);
    cmd._pMeshData = (!data ? pMesh->GetMeshDataLod() : data);
    R_ASSERT(cmd._pMeshData, "Mesh data was nullptr!");
 
    gRenderer().PushMeshRender(cmd);
  }

  if (model == renderData->_Model) return;
  Matrix4 N = model;
  N[3][0] = 0.0f;
  N[3][1] = 0.0f;
  N[3][2] = 0.0f;
  N[3][3] = 1.0f;
  renderData->_Model = model;
  renderData->_NormalMatrix = N.Inverse().Transpose();
  m_meshDescriptor->PushUpdate(MESH_BUFFER_UPDATE_BIT);
}


void RendererComponent::SetTransparent(b32 enable)
{
  if (enable) { m_configs |= CMD_TRANSPARENT_BIT | CMD_FORWARD_BIT; }
  else { m_configs &= ~(CMD_TRANSPARENT_BIT | CMD_FORWARD_BIT); }
}


b32 RendererComponent::TransparentEnabled() const
{
  return (m_configs & CMD_TRANSPARENT_BIT);
}


void SkinnedRendererComponent::Update()
{
  JointBuffer* pJointBuffer = m_pJointDescriptor->JointData();
  R_ASSERT(pJointBuffer, "Joint buffer found null!");
  u32 jointCount = m_pJointDescriptor->NumJoints();

  // TODO(): Need to extract joint pose matrices from animation
  // component!

  const Matrix4* palette = nullptr;
  u32 paletteSz = 0;
  if (m_pAnimHandle) {
    palette = m_pAnimHandle->_finalPalette;
    paletteSz = m_pAnimHandle->_paletteSz;
  }
  // Update descriptor joints.
  // use matrix palette K and sent to gpu for skinning. This is the bind pose model space.
  memcpy(pJointBuffer->_mJoints, palette, paletteSz * sizeof(Matrix4));
  m_pJointDescriptor->PushUpdate(JOINT_BUFFER_UPDATE_BIT);
  RendererComponent::Update();
}


void RendererComponent::EnableSkin(b32 enable)
{
  ObjectBuffer* buffer = m_meshDescriptor->ObjectData();
  buffer->_HasJoints = enable;
}


void SkinnedRendererComponent::OnInitialize(GameObject* owner)
{
  m_meshDescriptor = gRenderer().CreateMeshDescriptor();
  m_pJointDescriptor = gRenderer().CreateJointDescriptor();

  m_meshDescriptor->Initialize(gRenderer().RHI());
  m_pJointDescriptor->Initialize(gRenderer().RHI());

  m_meshDescriptor->PushUpdate(MESH_DESCRIPTOR_UPDATE_BIT);
  m_pJointDescriptor->PushUpdate(JOINT_DESCRIPTOR_UPDATE_BIT);

  // Set joints to true, since this renderer component is skinned.
  m_meshDescriptor->ObjectData()->_HasJoints = true;

  REGISTER_COMPONENT(SkinnedRendererComponent, this);
}


void SkinnedRendererComponent::OnCleanUp()
{
  gRenderer().FreeMeshDescriptor(m_meshDescriptor);
  gRenderer().FreeJointDescriptor(m_pJointDescriptor);
  m_meshDescriptor = nullptr;
  m_pJointDescriptor = nullptr;

  UNREGISTER_COMPONENT(SkinnedRendererComponent);
}


SkinnedRendererComponent::SkinnedRendererComponent()
  : m_pAnimHandle(nullptr)
  , m_pJointDescriptor(nullptr)
{
}


void RendererComponent::UpdateLod(Transform* meshTransform)
{
  if (!AllowLod()) return;
  Camera* currCamera = Camera::GetMain();
  if (!currCamera) return;
  Transform* camTransform = currCamera->GetTransform();
  Vector3 camPos = camTransform->Position;
  Vector3 meshPos = meshTransform->Position;

  // Length of vector between mesh and camera.
  r32 len = (meshPos - camPos).Magnitude();
  m_currLod = 0;
  if (len > 10.0f) {
    m_currLod = 1;
  }
  if (len > 15.0f) {
    m_currLod = 2;  
  }
  if (len > 20.0f) {
    m_currLod = 3;
  }
  if (len > 25.0f) {
    m_currLod = 4;
  }
}
} // Recluse