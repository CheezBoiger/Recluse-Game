// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "LightComponent.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Geometry/UVSphere.hpp"

#include "Core/Math/Vector4.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/LightDescriptor.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "PointLightComponent.hpp"


namespace Recluse {


std::queue<u32> PointLightComponent::sAvailablePointLightIds;
std::queue<u32> LightComponent::sAvailableDirectionalLightIds;
std::queue<u32> SpotLightComponent::sAvailableSpotLightIds;
Mesh*           kPointLightMesh = nullptr;
Primitive       pointLightPrim;

DEFINE_COMPONENT_MAP(PointLightComponent);
DEFINE_COMPONENT_MAP(SpotLightComponent);


void LightComponent::globalInitialize()
{
  u32 pointLightCount = LightBuffer::maxNumPointLights();
  u32 dirLightCount = LightBuffer::maxNumDirectionalLights();
  u32 spotLightCount = LightBuffer::maxNumSpotLights();

  for (u32 i = 0; i < pointLightCount; ++i) {
    PointLightComponent::sAvailablePointLightIds.push(i);
  }

  for (u32 i = 0; i < dirLightCount; ++i) {
    sAvailableDirectionalLightIds.push(i);
  }

  for (u32 i = 0; i < spotLightCount; ++i) {
    SpotLightComponent::sAvailableSpotLightIds.push(i);
  }

  PointLightComponent::InitializeMeshDebug();
}


void LightComponent::globalCleanUp()
{
  PointLightComponent::CleanUpMeshDebug();
}


void PointLightComponent::onInitialize(GameObject* owner)
{
  if (sAvailablePointLightIds.empty()) {
    R_DEBUG(rError, "Point light can not be assigned a point light! Max exceeded!\n");
    return;
  }
  m_Id = sAvailablePointLightIds.front();
  sAvailablePointLightIds.pop();

  LightBuffer* lights = gRenderer().getLightData();
  m_NativeLight = &lights->_PointLights[m_Id];
  m_NativeLight->_Enable = true;
  m_NativeLight->_Range = 1.0f;
  m_NativeLight->_Intensity = 1.0f;
  m_NativeLight->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

  REGISTER_COMPONENT(PointLightComponent, this);
}


void PointLightComponent::onCleanUp()
{
  if (m_Id >= LightBuffer::maxNumPointLights()) {
    return;
  }
  
  // Push back light id as it is now available.
  sAvailablePointLightIds.push(m_Id);
  m_NativeLight->_Enable = false;

  if (m_descriptor) {
    gRenderer().freeMeshDescriptor(m_descriptor);
    m_descriptor = nullptr;
  }

  UNREGISTER_COMPONENT(PointLightComponent);
}


void PointLightComponent::update()
{
  Transform* transform = getOwner()->getTransform();
  if (!transform) {
    return;
  }
  // Rotate about the same area of where the position should be.
  Vector3 rel = transform->_rotation * m_offset;
  m_NativeLight->_Position = transform->_position + rel;

  if (isDebugging()) {
    MeshRenderCmd cmd;
    cmd._config = CMD_RENDERABLE_BIT;
    cmd._pMeshDesc = m_descriptor;
    cmd._pMeshData = kPointLightMesh->getMeshData();
    cmd._pPrimitives = kPointLightMesh->getPrimitiveData();
    cmd._primitiveCount = kPointLightMesh->getPrimitiveCount();

    ObjectBuffer* buffer = m_descriptor->getObjectData();
    buffer->_model = Matrix4(
      Vector4(1.0f, 0.0f, 0.0f, 0.0f),
      Vector4(0.0f, 1.0f, 0.0f, 0.0f),
      Vector4(0.0f, 0.0f, 1.0f, 0.0f),
      m_NativeLight->_Position);
    buffer->_model[3][3] = 1.0f;
    buffer->_model = Matrix4::scale(buffer->_model, Vector3(0.3f, 0.3f, 0.3f));

    Matrix4 N = buffer->_model;
    N[3][0] = 0.0f;
    N[3][1] = 0.0f;
    N[3][2] = 0.0f;
    N[3][3] = 1.0f;
    buffer->_normalMatrix = N.inverse().transpose();
    m_descriptor->pushUpdate(MESH_BUFFER_UPDATE_BIT);
    gRenderer().pushMeshRender(cmd);
  }
}


void PointLightComponent::InitializeMeshDebug()
{
  kPointLightMesh = new Mesh();
  u32 g = 48;
  auto vertices = UVSphere::meshInstance(1.0f, g, g);
  auto indices = UVSphere::indicesInstance(static_cast<u32>(vertices.size()), g, g);
  kPointLightMesh->initialize(&gRenderer(), vertices.size(), vertices.data(), Mesh::STATIC, indices.size(), indices.data());
  pointLightPrim._firstIndex = 0;
  pointLightPrim._indexCount = 
    kPointLightMesh->getMeshData()->getIndexData()->IndexCount();
  pointLightPrim._pMat = Material::getDefault();
}


void PointLightComponent::CleanUpMeshDebug()
{
  kPointLightMesh->cleanUp(&gRenderer());
  delete kPointLightMesh;
}


void PointLightComponent::onDebug()
{
  if (m_debug && !m_descriptor) {
    m_descriptor = gRenderer().createMeshDescriptor();
    m_descriptor->initialize(gRenderer().getRHI());
    m_descriptor->pushUpdate(MESH_DESCRIPTOR_UPDATE_BIT);
  } else if (!m_debug && m_descriptor) {
    gRenderer().freeMeshDescriptor(m_descriptor);
    m_descriptor = nullptr;
  }
}


void SpotLightComponent::onCleanUp()
{
  if (m_Id >= LightBuffer::maxNumSpotLights()) {
    return;
  }
  
  // Push back light id as it is now available.
  sAvailableSpotLightIds.push(m_Id);
  m_NativeLight->_Enable = false;

  UNREGISTER_COMPONENT(SpotLightComponent);
}


void SpotLightComponent::onInitialize(GameObject* owner)
{
  if (sAvailableSpotLightIds.empty()) {
    R_DEBUG(rError, "Spot light can not be assigned a spot light! Max exceeded!\n");
    return;
  }
  m_Id = sAvailableSpotLightIds.front();
  sAvailableSpotLightIds.pop();

  LightBuffer* lights = gRenderer().getLightData();
  m_NativeLight = &lights->_SpotLights[m_Id];
  m_NativeLight->_Enable = true;
  m_NativeLight->_Range = 10.0f;
  m_NativeLight->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  m_NativeLight->_InnerCutOff = cosf(Radians(12.7f));
  m_NativeLight->_OuterCutOff = cosf(Radians(17.5f));

  REGISTER_COMPONENT(SpotLightComponent, this);
}


void SpotLightComponent::update()
{
  Transform* transform = getOwner()->getTransform();
  if (!transform) {
    return;
  }

  if (!m_fixed) {
    // Rotate about the same area of where the position should be.
    if (m_syncGameObject) {
      Vector3 rel = transform->_rotation * m_offset;
      m_NativeLight->_Position = transform->_position + rel;
      m_NativeLight->_Direction = transform->front() * m_rotQuat;
    }
  } else {
    // Fixed transformation. No Relative rotation to the parent mesh.
    m_NativeLight->_Position = transform->_position + m_offset;
    m_NativeLight->_Direction = Vector3::FRONT * m_rotQuat;
  }
}
} // Recluse