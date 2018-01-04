// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshComponent.hpp"

#include "Renderer/RenderObject.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Exception.hpp"

namespace Recluse {


MeshComponent::MeshComponent()
  : mRenderer(nullptr)
  , mMaterial(nullptr)
  , mRenderObj(nullptr)
  , mMeshDescriptor(nullptr)
{
}


MeshComponent::MeshComponent(const MeshComponent& m)
  : mRenderer(m.mRenderer)
  , mMaterial(m.mMaterial)
  , mMeshDescriptor(m.mMeshDescriptor)
  , mRenderObj(m.mRenderObj)
{
}


MeshComponent::MeshComponent(MeshComponent&& m)
  : mRenderer(m.mRenderer)
  , mMaterial(m.mMaterial)
  , mMeshDescriptor(m.mMeshDescriptor)
  , mRenderObj(m.mRenderObj)
{
  m.mMaterial = nullptr;
  m.mMeshDescriptor = nullptr;
  m.mRenderer = nullptr;
  m.mRenderObj = nullptr;
}


MeshComponent& MeshComponent::operator=(MeshComponent&& obj)
{
  mRenderer = obj.mRenderer;
  mRenderObj = obj.mRenderObj;
  mMaterial = obj.mMaterial;
  mMeshDescriptor = obj.mMeshDescriptor;

  obj.mMeshDescriptor = nullptr;
  obj.mMaterial = nullptr;
  obj.mRenderObj = nullptr;
  obj.mRenderer = nullptr;
  return (*this);
}


MeshComponent& MeshComponent::operator=(const MeshComponent& obj)
{
  mRenderer = obj.mRenderer;
  mRenderObj = obj.mRenderObj;
  mMaterial = obj.mMaterial;
  mMeshDescriptor = obj.mMeshDescriptor;
  return (*this);
}
} // Recluse