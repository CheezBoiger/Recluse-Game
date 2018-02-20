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


RendererComponent::RendererComponent()
  : mRenderObj(nullptr)
  , mMeshDescriptor(nullptr)
{
}


RendererComponent::RendererComponent(const RendererComponent& m)
  : mMeshDescriptor(m.mMeshDescriptor)
  , mRenderObj(m.mRenderObj)
{
}


RendererComponent::RendererComponent(RendererComponent&& m)
  : mMeshDescriptor(m.mMeshDescriptor)
  , mRenderObj(m.mRenderObj)
{
  m.mMeshDescriptor = nullptr;
  m.mRenderObj = nullptr;
}


RendererComponent& RendererComponent::operator=(RendererComponent&& obj)
{
  mRenderObj = obj.mRenderObj;
  mMeshDescriptor = obj.mMeshDescriptor;

  obj.mMeshDescriptor = nullptr;
  obj.mRenderObj = nullptr;
  return (*this);
}


RendererComponent& RendererComponent::operator=(const RendererComponent& obj)
{
  mRenderObj = obj.mRenderObj;
  mMeshDescriptor = obj.mMeshDescriptor;
  return (*this);
}


void RendererComponent::ReConfigure()
{
  MeshComponent* meshc = GetOwner()->GetComponent<MeshComponent>();
  if (meshc) {
    mRenderObj->ClearOutMeshGroup();
    mRenderObj->PushBack(meshc->MeshRef()->Native());
  }

  MaterialComponent* materialComponent = GetOwner()->GetComponent<MaterialComponent>();
  Material* material = Material::Default();
  if (materialComponent) {
    material = materialComponent->GetMaterial();
  }

  mRenderObj->_pMaterialDescId = material->Native();
  mRenderObj->Update();
  mRenderObj->SwapDescriptorSet();
}


void RendererComponent::OnInitialize(GameObject* owner)
{
  if (mRenderObj) {
    Log(rWarning) << "Renderer Component is already initialized! Skipping...\n";
    return;
  }

  MaterialComponent* mc = owner->GetComponent<MaterialComponent>();
  Material* material = Material::Default();
  if (mc) {
    material = mc->GetMaterial();
  }
  
  mRenderObj = gRenderer().CreateRenderObject();
  mMeshDescriptor = gRenderer().CreateStaticMeshDescriptor();
  mMeshDescriptor->Initialize();

  mRenderObj->_pMeshDescId = mMeshDescriptor;
  mRenderObj->_pMaterialDescId = material->Native();
  mRenderObj->Initialize();

  // Check if MeshComponent is in game object.
  MeshComponent* mesh = owner->GetComponent<MeshComponent>();
  if (mesh) {
    mRenderObj->PushBack(mesh->MeshRef()->Native());
  }
}


void RendererComponent::OnCleanUp()
{
  gRenderer().FreeRenderObject(mRenderObj);
  gRenderer().FreeMeshDescriptor(mMeshDescriptor);
  mRenderObj = nullptr;
  mMeshDescriptor = nullptr;
}


void RendererComponent::Enable(b8 enable)
{
  mRenderObj->Renderable = enable;
}


b8 RendererComponent::Enabled() const
{
  return mRenderObj->Renderable;
}


void RendererComponent::Update()
{
  // TODO(): Static objects don't necessarily need to be updated.
  Transform* transform = GetOwner()->GetComponent<Transform>();
  if (transform) {
    ObjectBuffer* renderData = mMeshDescriptor->ObjectData();
    Matrix4 model = transform->GetLocalToWorldMatrix();
    renderData->_Model = model;
    renderData->_NormalMatrix = renderData->_Model.Inverse().Transpose();
    renderData->_NormalMatrix[3][0] = 0.0f;
    renderData->_NormalMatrix[3][1] = 0.0f;
    renderData->_NormalMatrix[3][2] = 0.0f;
    renderData->_NormalMatrix[3][3] = 1.0f;

    mMeshDescriptor->Update();
  }
}
} // Recluse