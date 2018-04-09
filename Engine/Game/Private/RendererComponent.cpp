// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererComponent.hpp"
#include "MeshComponent.hpp"
#include "MaterialComponent.hpp"
#include "GameObject.hpp"

#include "Renderer/RenderObject.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


DEFINE_COMPONENT_MAP(RendererComponent);


RendererComponent::RendererComponent()
  : m_renderObj(nullptr)
  , m_meshDescriptor(nullptr)
  , m_materialRef(nullptr)
  , m_meshRef(nullptr)
{
}


RendererComponent::RendererComponent(const RendererComponent& m)
  : m_meshDescriptor(m.m_meshDescriptor)
  , m_renderObj(m.m_renderObj)
  , m_materialRef(m.m_materialRef)
  , m_meshRef(m.m_meshRef)
{
}


RendererComponent::RendererComponent(RendererComponent&& m)
  : m_meshDescriptor(m.m_meshDescriptor)
  , m_renderObj(m.m_renderObj)
  , m_materialRef(m.m_materialRef)
  , m_meshRef(m.m_meshRef)
{
  m.m_meshDescriptor = nullptr;
  m.m_renderObj = nullptr;
  m.m_materialRef = nullptr;
  m.m_meshRef = nullptr;
}


RendererComponent& RendererComponent::operator=(RendererComponent&& obj)
{
  m_renderObj = obj.m_renderObj;
  m_meshDescriptor = obj.m_meshDescriptor;
  m_materialRef = obj.m_materialRef;
  m_meshRef = obj.m_meshRef;

  obj.m_meshDescriptor = nullptr;
  obj.m_renderObj = nullptr;
  obj.m_materialRef = nullptr;
  obj.m_meshRef = nullptr;
  return (*this);
}


RendererComponent& RendererComponent::operator=(const RendererComponent& obj)
{
  m_renderObj = obj.m_renderObj;
  m_meshDescriptor = obj.m_meshDescriptor;
  m_meshRef = obj.m_meshRef;
  m_materialRef = obj.m_materialRef;
  return (*this);
}


void RendererComponent::EnableShadow(b8 enable)
{
  m_renderObj->_bEnableShadow = enable;
}


b8 RendererComponent::ShadowEnabled() const
{
  return m_renderObj->_bEnableShadow;
}


void RendererComponent::ReConfigure()
{
  m_renderObj->ClearOutMeshGroup();
  if (m_meshRef && m_meshRef->MeshRef()) {
    m_renderObj->PushBack(m_meshRef->MeshRef()->Native());
  }

  Material* material = Material::Default();
  if (m_materialRef) {
    material = m_materialRef->GetMaterial();
  }

  m_renderObj->SetMaterialDescriptor( material->Native() );
}


void RendererComponent::OnInitialize(GameObject* owner)
{
  if (m_renderObj) {
    Log(rWarning) << "Renderer Component is already initialized! Skipping...\n";
    return;
  }

  Material* material = Material::Default();
  if (m_materialRef && m_materialRef->GetMaterial()) {
    material = m_materialRef->GetMaterial();
  }
  
  m_renderObj = gRenderer().CreateRenderObject(owner->GetId());
  m_meshDescriptor = gRenderer().CreateStaticMeshDescriptor();
  m_meshDescriptor->Initialize();

  m_renderObj->SetMeshDescriptor( m_meshDescriptor );
  m_renderObj->SetMaterialDescriptor( material->Native() );
  m_renderObj->Initialize();

  // Check if MeshComponent is in game object.
  if (m_meshRef && m_meshRef->MeshRef()) {
    m_renderObj->PushBack(m_meshRef->MeshRef()->Native());
  }

  REGISTER_COMPONENT(RendererComponent, this);
}


void RendererComponent::OnCleanUp()
{
  gRenderer().FreeRenderObject(m_renderObj);
  gRenderer().FreeMeshDescriptor(m_meshDescriptor);
  m_renderObj = nullptr;
  m_meshDescriptor = nullptr;

  UNREGISTER_COMPONENT(RendererComponent);
}


void RendererComponent::Enable(b8 enable)
{
  m_renderObj->Renderable = enable;
}


b8 RendererComponent::Enabled() const
{
  return m_renderObj->Renderable;
}


void RendererComponent::Update()
{
  // TODO(): Static objects don't necessarily need to be updated.
  Transform* transform = GetOwner()->GetTransform();
  if (transform) {
    ObjectBuffer* renderData = m_meshDescriptor->ObjectData();
    Matrix4 model = transform->GetLocalToWorldMatrix();
    if (renderData->_Model != model) {
      renderData->_Model = model;
      renderData->_NormalMatrix = renderData->_Model.Inverse().Transpose();
      renderData->_NormalMatrix[3][0] = 0.0f;
      renderData->_NormalMatrix[3][1] = 0.0f;
      renderData->_NormalMatrix[3][2] = 0.0f;
      renderData->_NormalMatrix[3][3] = 1.0f;
      m_meshDescriptor->SignalUpdate();
    }
  }
}
} // Recluse