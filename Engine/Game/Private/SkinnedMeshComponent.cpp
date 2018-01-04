// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "SkinnedMeshComponent.hpp"

#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/RenderObject.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


SkinnedMeshComponent::SkinnedMeshComponent()
  : mRenderer(nullptr)
  , mMaterial(nullptr)
  , mRenderObj(nullptr)
  , mMesh(nullptr)
{
}


SkinnedMeshComponent::SkinnedMeshComponent(const SkinnedMeshComponent& obj)
  : mRenderer(obj.mRenderer)
  , mMaterial(obj.mMaterial)
  , mRenderObj(obj.mRenderObj)
  , mMesh(obj.mMesh)
{
}


SkinnedMeshComponent::SkinnedMeshComponent(SkinnedMeshComponent&& obj)
  : mRenderer(obj.mRenderer)
  , mMaterial(obj.mMaterial)
  , mRenderObj(obj.mRenderObj)
  , mMesh(obj.mMesh)
{
  obj.mRenderer = nullptr;
  obj.mMaterial = nullptr;
  obj.mMesh = nullptr;
  obj.mRenderObj = nullptr;
}


SkinnedMeshComponent& SkinnedMeshComponent::operator=(const SkinnedMeshComponent& obj)
{
  mRenderer = obj.mRenderer;
  mRenderObj = obj.mRenderObj;
  mMaterial = obj.mMaterial;
  mMesh = obj.mMesh;

  return (*this);
}


SkinnedMeshComponent& SkinnedMeshComponent::operator=(SkinnedMeshComponent&& obj)
{
  mRenderer = obj.mRenderer;
  mRenderObj = obj.mRenderObj;
  mMaterial = obj.mMaterial;
  mMesh = obj.mMesh;

  obj.mRenderer = nullptr;
  obj.mMaterial = nullptr;
  obj.mRenderObj = nullptr;
  obj.mMesh = nullptr;
  return (*this);
}


SkinnedMeshComponent::~SkinnedMeshComponent()
{
  if (mRenderObj) {
    R_DEBUG(rWarning, "This skinned mesh component has not cleaned up it's render object prior to destruction!\n");
  }
}


void SkinnedMeshComponent::Initialize(Renderer* renderer, MaterialDescriptor* material, SkinnedMeshDescriptor* descriptor)
{
  if (!renderer || !material || !descriptor) {
    R_DEBUG(rNotify, "Null parameters for this skinned mesh component.\n");
    return;
  }

  if (mRenderObj) {
    R_DEBUG(rNotify, "A render object already exists for this skinned mesh component. Skipping...\n");
    return;
  }
  mRenderer = renderer;
  mMaterial = material;
  mMesh = descriptor;

  mRenderObj = mRenderer->CreateRenderObject();

  mRenderObj->MaterialId = mMaterial;
  mRenderObj->MeshDescriptorId = mMesh;

  mRenderObj->Initialize();
}


void SkinnedMeshComponent::CleanUp()
{
  if (mRenderObj) {
    mRenderer->FreeRenderObject(mRenderObj);
    mRenderObj = nullptr;
  }
}
} // Recluse