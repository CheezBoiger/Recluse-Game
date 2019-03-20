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


DEFINE_COMPONENT_MAP(AbstractRendererComponent);


AbstractRendererComponent::AbstractRendererComponent()
  : m_configs(CMD_RENDERABLE_BIT | CMD_SHADOWS_BIT)
  , m_debugConfigs(0)
  , m_bDirty(true)
  , m_currLod(Mesh::kMeshLodZero)
  , m_allowLod(true)
  , m_morphIndex0(kNoMorphIndex)
  , m_morphIndex1(kNoMorphIndex)
  , m_pAnimHandle(nullptr)
{
}


RendererComponent::RendererComponent()
  : m_meshDescriptor(nullptr)
  , AbstractRendererComponent()
{
}


RendererComponent::RendererComponent(const RendererComponent& m)
  : m_meshDescriptor(m.m_meshDescriptor)
{
  m_meshes = m.m_meshes;
}


RendererComponent::RendererComponent(RendererComponent&& m)
  : m_meshDescriptor(m.m_meshDescriptor)
{
  m.m_meshDescriptor = nullptr;
  m_meshes = std::move(m.m_meshes);
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


void AbstractRendererComponent::enableShadow(b32 enable)
{
  if (enable) { m_configs |= CMD_SHADOWS_BIT; }
  else { m_configs &= ~CMD_SHADOWS_BIT; }
}

void AbstractRendererComponent::enableDebug(b32 enable)
{
  if (enable) { m_configs |= CMD_DEBUG_BIT; }
  else { m_configs &= ~CMD_DEBUG_BIT; }
}


b32 AbstractRendererComponent::isShadowEnabled() const
{
  return (m_configs & CMD_SHADOWS_BIT);
}


void AbstractRendererComponent::enableStatic(b32 enable)
{
  if (enable) { m_configs |= CMD_STATIC_BIT; }
  else { m_configs &= ~CMD_STATIC_BIT; }
}

void RendererComponent::onInitialize(GameObject* owner)
{ 
  m_meshDescriptor = gRenderer().createMeshDescriptor();
  m_meshDescriptor->initialize(gRenderer().getRHI());
  m_meshDescriptor->pushUpdate(MESH_DESCRIPTOR_UPDATE_BIT);

  REGISTER_COMPONENT(AbstractRendererComponent, this);
}


void RendererComponent::onCleanUp()
{
  gRenderer().freeMeshDescriptor(m_meshDescriptor);
  m_meshDescriptor = nullptr;

  UNREGISTER_COMPONENT(AbstractRendererComponent);
}


void AbstractRendererComponent::onEnable()
{
  if (enabled()) { m_configs |= CMD_RENDERABLE_BIT; }
  else { m_configs &= ~CMD_RENDERABLE_BIT; }
}


void AbstractRendererComponent::forceForward(b32 enable)
{
  if (enable) { m_configs |= CMD_FORWARD_BIT; }
  else { m_configs &= ~CMD_FORWARD_BIT; }
}


void AbstractRendererComponent::enableMorphTargets(b32 enable)
{
  if (enable) { m_configs |= CMD_MORPH_BIT; }
  else { m_configs &= ~CMD_MORPH_BIT; }
}


void AbstractRendererComponent::setDebugBits(b32 bits)
{
  m_debugConfigs |= bits;
}

void AbstractRendererComponent::unsetDebugBits(b32 bits)
{
  m_debugConfigs &= ~bits;
}


void RendererComponent::update()
{
  if (!enabled() || m_meshes.empty()) return;
  // TODO(): isStatic objects don't necessarily need to be updated all the time.
  // This is especially true if the object is kinematic
  Transform* transform = getOwner()->getTransform();
  ObjectBuffer* renderData = m_meshDescriptor->getObjectData();
  Matrix4 model = transform->getLocalToWorldMatrix();
  
  updateLod(transform);

  // Now push the object into the renderer for updating.
  for (size_t i = 0; i < m_meshes.size(); ++i) {
    MeshRenderCmd cmd;
    cmd._pMeshDesc = m_meshDescriptor;
    cmd._pJointDesc = getJointDescriptor();
    cmd._config = m_configs;
    cmd._debugConfig = m_debugConfigs;

    // Push mesh data to renderer.
    Mesh* pMesh = m_meshes[i];
    MeshData* data = pMesh->getMeshData();
    cmd._pMeshData = data;
    cmd._pPrimitives = pMesh->getPrimitiveData();
    cmd._primitiveCount = pMesh->getPrimitiveCount();

    R_ASSERT(cmd._pMeshData, "Mesh data was nullptr!");
    if (m_meshes[i]->getMorphTargetCount() > 0) {
      cmd._pMorph0 = m_meshes[i]->getMorphTarget(0);
      cmd._pMorph1 = m_meshes[i]->getMorphTarget(1);
    }

    gRenderer().pushMeshRender(cmd);
  }

  if ( m_pAnimHandle ) {
    const r32* weights = nullptr;
    u32 weightSz = 0;
    weights = m_pAnimHandle->_finalMorphs.data();
    weightSz = static_cast<u32>(m_pAnimHandle->_finalMorphs.size());
    if (weightSz > 0) {
      renderData->_w0 = weights[0];
      renderData->_w1 = weights[1];
    }
  }

  if (model == renderData->_model) return;
  Matrix4 N = model;
  N[3][0] = 0.0f;
  N[3][1] = 0.0f;
  N[3][2] = 0.0f;
  N[3][3] = 1.0f;
  renderData->_model = model;
  renderData->_normalMatrix = N.inverse().transpose();
  m_meshDescriptor->pushUpdate(MESH_BUFFER_UPDATE_BIT);
}


void AbstractRendererComponent::setTransparent(b32 enable)
{
  if (enable) { m_configs |= CMD_TRANSPARENT_BIT | CMD_FORWARD_BIT; }
  else { m_configs &= ~(CMD_TRANSPARENT_BIT | CMD_FORWARD_BIT); }
}


b32 AbstractRendererComponent::isTransparentEnabled() const
{
  return (m_configs & CMD_TRANSPARENT_BIT);
}


void SkinnedRendererComponent::update()
{
  JointBuffer* pJointBuffer = m_pJointDescriptor->getJointData();
  R_ASSERT(pJointBuffer, "Joint buffer found null!");
  u32 jointCount = m_pJointDescriptor->numJoints();

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
  if ( palette ) {
    memcpy(pJointBuffer->_mJoints, palette, paletteSz * sizeof(Matrix4));
  } else {
    memcpy(pJointBuffer->_mJoints, JointBuffer::defaultMatrices, sizeof(Matrix4) * JointBuffer::kMaxNumberOfJointMatrices);
  }

  m_pJointDescriptor->pushUpdate(JOINT_BUFFER_UPDATE_BIT);
  
  RendererComponent::update();
}


void RendererComponent::enableSkin(b32 enable)
{
  ObjectBuffer* buffer = m_meshDescriptor->getObjectData();
  buffer->_hasJoints = enable;
}


void SkinnedRendererComponent::onInitialize(GameObject* owner)
{
  m_meshDescriptor = gRenderer().createMeshDescriptor();
  m_pJointDescriptor = gRenderer().createJointDescriptor();

  m_meshDescriptor->initialize(gRenderer().getRHI());
  m_pJointDescriptor->initialize(gRenderer().getRHI());

  m_meshDescriptor->pushUpdate(MESH_DESCRIPTOR_UPDATE_BIT);
  m_pJointDescriptor->pushUpdate(JOINT_DESCRIPTOR_UPDATE_BIT);

  // Set joints to true, since this renderer component is skinned.
  m_meshDescriptor->getObjectData()->_hasJoints = true;
  // Set the command to be skinned, since this mesh object is skinned.
  m_configs |= CMD_SKINNED_BIT;

  REGISTER_COMPONENT(AbstractRendererComponent, this);
}


void SkinnedRendererComponent::onCleanUp()
{
  gRenderer().freeMeshDescriptor(m_meshDescriptor);
  gRenderer().freeJointDescriptor(m_pJointDescriptor);
  m_meshDescriptor = nullptr;
  m_pJointDescriptor = nullptr;

  UNREGISTER_COMPONENT(AbstractRendererComponent);
}


SkinnedRendererComponent::SkinnedRendererComponent()
  : m_pJointDescriptor(nullptr)
{
}


void AbstractRendererComponent::updateLod(Transform* meshTransform)
{
  if (!allowLod()) return;
  Camera* currCamera = Camera::getMain();
  if (!currCamera) return;
  Transform* camTransform = currCamera->getTransform();
  Vector3 camPos = camTransform->_position;
  Vector3 meshPos = meshTransform->_position;

  // Length of vector between mesh and camera.
  r32 len = (meshPos - camPos).length();
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


void BatchRendererComponent::onInitialize(GameObject* owner)
{
  //
  REGISTER_COMPONENT(AbstractRendererComponent, this);
}


void BatchRendererComponent::addMesh(Mesh* pMeshRef)
{
  AbstractRendererComponent::addMesh(pMeshRef);

  MeshDescriptor* pMeshDescriptor = gRenderer().createMeshDescriptor();
  pMeshDescriptor->initialize(gRenderer().getRHI());
  pMeshDescriptor->pushUpdate(MESH_DESCRIPTOR_UPDATE_BIT);
  m_perMeshDescriptors.push_back(pMeshDescriptor);
}


void BatchRendererComponent::clearMeshes()
{
  AbstractRendererComponent::clearMeshes();
  for (u32 i = 0; i < m_perMeshDescriptors.size(); ++i) {
    gRenderer().freeMeshDescriptor(m_perMeshDescriptors[i]);
    m_perMeshDescriptors[i] = nullptr;
  }
  m_perMeshDescriptors.clear();
}


void BatchRendererComponent::update()
{
  Transform* transform = getTransform();
  Matrix4 parentModel = transform->getLocalToWorldMatrix();

  // Each mesh corresponds to each mesh descriptor.
  for (u32 i = 0; i < m_perMeshDescriptors.size(); ++i) {
    MeshRenderCmd meshCmd = { };
    MeshDescriptor* pMeshDescriptor = m_perMeshDescriptors[i];
    ObjectBuffer* pBuffer = pMeshDescriptor->getObjectData();
    MeshData* pMeshData = m_meshes[i]->getMeshData();
    Matrix4 localMatrix = Matrix4::identity();

    meshCmd._pMeshData = pMeshData;
    meshCmd._pMeshDesc = pMeshDescriptor;
    meshCmd._pJointDesc = getJointDescriptor(i);
    meshCmd._config = m_configs;
    meshCmd._debugConfig = m_debugConfigs;
    meshCmd._instances = 1;
    meshCmd._primitiveCount = m_meshes[i]->getPrimitiveCount();
    meshCmd._pPrimitives = m_meshes[i]->getPrimitiveData();

    R_ASSERT(meshCmd._pMeshData, "Mesh data was nullptr!");
    if (m_meshes[i]->getMorphTargetCount() > 0) {
      meshCmd._pMorph0 = m_meshes[i]->getMorphTarget(0);
      meshCmd._pMorph1 = m_meshes[i]->getMorphTarget(1);
    }

    if (m_pAnimHandle) {
      const r32* weights = nullptr;
      u32 weightSz = 0;
      weights = m_pAnimHandle->_finalMorphs.data();
      weightSz = static_cast<u32>(m_pAnimHandle->_finalMorphs.size());
      if (weightSz > 0) {
        pBuffer->_w0 = weights[0];
        pBuffer->_w1 = weights[1];
      }
      localMatrix = m_pAnimHandle->_finalPalette[i];
    }

    Matrix4 model = localMatrix * parentModel;
    Matrix4 N = model;
    N[3][0] = 0.0f; 
    N[3][1] = 0.0f;
    N[3][2] = 0.0f; 
    N[3][3] = 1.0f;
    pBuffer->_model = model;
    pBuffer->_normalMatrix = N.inverse().transpose();
    pMeshDescriptor->pushUpdate(MESH_BUFFER_UPDATE_BIT);
    gRenderer().pushMeshRender(meshCmd);
  }
}


void BatchRendererComponent::onCleanUp()
{
  for (u32 i = 0; i < m_perMeshDescriptors.size(); ++i) {
    m_perMeshDescriptors[i]->cleanUp(gRenderer().getRHI());
    m_perMeshDescriptors[i] = nullptr;
  }
  m_perMeshDescriptors.clear();

  UNREGISTER_COMPONENT(AbstractRendererComponent);
}
} // Recluse