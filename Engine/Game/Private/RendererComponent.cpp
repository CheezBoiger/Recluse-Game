// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererComponent.hpp"

#include "Renderer/RenderObject.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


RendererComponent::RendererComponent()
  : mMaterial(nullptr)
  , mRenderObj(nullptr)
  , mMeshDescriptor(nullptr)
{
}


RendererComponent::RendererComponent(const RendererComponent& m)
  : mMaterial(m.mMaterial)
  , mMeshDescriptor(m.mMeshDescriptor)
  , mRenderObj(m.mRenderObj)
{
}


RendererComponent::RendererComponent(RendererComponent&& m)
  : mMaterial(m.mMaterial)
  , mMeshDescriptor(m.mMeshDescriptor)
  , mRenderObj(m.mRenderObj)
{
  m.mMaterial = nullptr;
  m.mMeshDescriptor = nullptr;
  m.mRenderObj = nullptr;
}


RendererComponent& RendererComponent::operator=(RendererComponent&& obj)
{
  mRenderObj = obj.mRenderObj;
  mMaterial = obj.mMaterial;
  mMeshDescriptor = obj.mMeshDescriptor;

  obj.mMeshDescriptor = nullptr;
  obj.mMaterial = nullptr;
  obj.mRenderObj = nullptr;
  return (*this);
}


RendererComponent& RendererComponent::operator=(const RendererComponent& obj)
{
  mRenderObj = obj.mRenderObj;
  mMaterial = obj.mMaterial;
  mMeshDescriptor = obj.mMeshDescriptor;
  return (*this);
}


void RendererComponent::OnInitialize(GameObject* owner)
{
  if (mRenderObj) {
    Log(rWarning) << "Renderer Component is already initialized! Skipping...\n";
    return;
  }
  mRenderObj = gRenderer().CreateRenderObject();
  mMaterial = gRenderer().CreateMaterialDescriptor();
  mMeshDescriptor = gRenderer().CreateStaticMeshDescriptor();
  mMaterial->Initialize();
  mMeshDescriptor->Initialize();

  mRenderObj->MeshDescriptorId = mMeshDescriptor;
  mRenderObj->MaterialId = mMaterial;
  mRenderObj->Initialize();
}


void RendererComponent::OnCleanUp()
{
  gRenderer().FreeRenderObject(mRenderObj);
  gRenderer().FreeMaterialDescriptor(mMaterial);
  gRenderer().FreeMeshDescriptor(mMeshDescriptor);
  mRenderObj = nullptr;
  mMaterial = nullptr;
  mMeshDescriptor = nullptr;
}
} // Recluse