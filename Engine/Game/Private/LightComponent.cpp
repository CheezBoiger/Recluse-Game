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


Mesh*           kPointLightMesh = nullptr;
Primitive       pointLightPrim;

DEFINE_COMPONENT_MAP(PointLightComponent);
DEFINE_COMPONENT_MAP(SpotLightComponent);


void LightComponent::globalInitialize()
{
  PointLightComponent::InitializeMeshDebug();
}


void LightComponent::globalCleanUp()
{
  PointLightComponent::CleanUpMeshDebug();
}


void PointLightComponent::onInitialize(GameObject* owner)
{

  REGISTER_COMPONENT(PointLightComponent, this);
}


void PointLightComponent::onCleanUp()
{ 
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
  m_nativeLight._Position = transform->_position + rel;

  gRenderer().pushPointLight(m_nativeLight);

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
      m_nativeLight._Position);
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
  UNREGISTER_COMPONENT(SpotLightComponent);
}


void SpotLightComponent::onInitialize(GameObject* owner)
{
  m_nativeLight._Enable = true;
  m_nativeLight._Range = 10.0f;
  m_nativeLight._Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  m_nativeLight._InnerCutOff = cosf(Radians(12.7f));
  m_nativeLight._OuterCutOff = cosf(Radians(17.5f));

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
      m_nativeLight._Position = transform->_position + rel;
      m_nativeLight._Direction = transform->front() * m_rotQuat;
    }
  } else {
    // Fixed transformation. No Relative rotation to the parent mesh.
    m_nativeLight._Position = transform->_position + m_offset;
    m_nativeLight._Direction = Vector3::FRONT * m_rotQuat;
  }

  gRenderer().pushSpotLight(m_nativeLight);
}
} // Recluse