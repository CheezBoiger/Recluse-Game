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
Mesh*           kPointLightMesh = nullptr;
Primitive       pointLightPrim;

DEFINE_COMPONENT_MAP(PointLightComponent);


void LightComponent::GlobalInitialize()
{
  u32 pointLightCount = LightBuffer::MaxNumPointLights();
  u32 dirLightCount = LightBuffer::MaxNumDirectionalLights();

  for (u32 i = 0; i < pointLightCount; ++i) {
    PointLightComponent::sAvailablePointLightIds.push(i);
  }

  for (u32 i = 0; i < dirLightCount; ++i) {
    sAvailableDirectionalLightIds.push(i);
  }

  PointLightComponent::InitializeMeshDebug();
}


void LightComponent::GlobalCleanUp()
{
  PointLightComponent::CleanUpMeshDebug();
}


void PointLightComponent::OnInitialize(GameObject* owner)
{
  if (sAvailablePointLightIds.empty()) {
    R_DEBUG(rError, "Point light can not be assigned a point light! Max exceeded!\n");
    return;
  }
  m_Id = sAvailablePointLightIds.front();
  sAvailablePointLightIds.pop();

  LightBuffer* lights = gRenderer().LightData();
  m_NativeLight = &lights->_PointLights[m_Id];
  m_NativeLight->_Enable = true;
  m_NativeLight->_Range = 1.0f;
  m_NativeLight->_Intensity = 1.0f;
  m_NativeLight->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

  REGISTER_COMPONENT(PointLightComponent, this);
}


void PointLightComponent::OnCleanUp()
{
  if (m_Id >= LightBuffer::MaxNumPointLights()) {
    return;
  }
  
  // Push back light id as it is now available.
  sAvailablePointLightIds.push(m_Id);
  m_NativeLight->_Enable = false;

  if (m_descriptor) {
    gRenderer().FreeMeshDescriptor(m_descriptor);
    m_descriptor = nullptr;
  }

  UNREGISTER_COMPONENT(PointLightComponent);
}


void PointLightComponent::Update()
{
  Transform* transform = GetOwner()->GetTransform();
  if (!transform) {
    return;
  }
  // Rotate about the same area of where the position should be.
  Vector3 rel = transform->Rotation * m_offset;
  m_NativeLight->_Position = transform->Position + rel;

  if (Debugging()) {
    MeshRenderCmd cmd;
    cmd._config = CMD_RENDERABLE_BIT;
    cmd._pMeshDesc = m_descriptor;
    cmd._pMeshData = kPointLightMesh->Native();
    cmd._lod = MESH_LOD_0;

    ObjectBuffer* buffer = m_descriptor->ObjectData();
    buffer->_Model = Matrix4(
      Vector4(1.0f, 0.0f, 0.0f, 0.0f),
      Vector4(0.0f, 1.0f, 0.0f, 0.0f),
      Vector4(0.0f, 0.0f, 1.0f, 0.0f),
      m_NativeLight->_Position);
    buffer->_Model[3][3] = 1.0f;
    buffer->_Model = Matrix4::Scale(buffer->_Model, Vector3(0.3f, 0.3f, 0.3f));

    Matrix4 N = buffer->_Model;
    N[3][0] = 0.0f;
    N[3][1] = 0.0f;
    N[3][2] = 0.0f;
    N[3][3] = 1.0f;
    buffer->_NormalMatrix = N.Inverse().Transpose();
    m_descriptor->PushUpdate(MESH_BUFFER_UPDATE);
    gRenderer().PushMeshRender(cmd);
  }
}


void PointLightComponent::InitializeMeshDebug()
{
  kPointLightMesh = new Mesh();
  u32 g = 48;
  auto vertices = UVSphere::MeshInstance(1.0f, g, g);
  auto indices = UVSphere::IndicesInstance(static_cast<u32>(vertices.size()), g, g);
  kPointLightMesh->Initialize(MESH_LOD_0, vertices.size(), vertices.data(), MeshData::STATIC, indices.size(), indices.data());
  pointLightPrim._firstIndex = 0;
  pointLightPrim._indexCount = 
    kPointLightMesh->Native()->IndexData(MESH_LOD_0)->IndexCount();
  pointLightPrim._pMat = Material::Default()->Native();
}


void PointLightComponent::CleanUpMeshDebug()
{
  kPointLightMesh->CleanUp();
  delete kPointLightMesh;
}


void PointLightComponent::OnDebug()
{
  if (m_debug && !m_descriptor) {
    m_descriptor = gRenderer().CreateMeshDescriptor();
    m_descriptor->Initialize(gRenderer().RHI());
    m_descriptor->PushUpdate(MESH_DESCRIPTOR_UPDATE);
  } else if (!m_debug && m_descriptor) {
    gRenderer().FreeMeshDescriptor(m_descriptor);
    m_descriptor = nullptr;
  }
}
} // Recluse