// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "RendererComponent.hpp"

#include "Renderer/RenderObject.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Exception.hpp"

namespace Recluse {


RendererComponent::RendererComponent()
  : mRenderer(nullptr)
  , mMaterial(nullptr)
  , mRenderObj(nullptr)
  , mMeshDescriptor(nullptr)
{
}


RendererComponent::RendererComponent(const RendererComponent& m)
  : mRenderer(m.mRenderer)
  , mMaterial(m.mMaterial)
  , mMeshDescriptor(m.mMeshDescriptor)
  , mRenderObj(m.mRenderObj)
{
}


RendererComponent::RendererComponent(RendererComponent&& m)
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


RendererComponent& RendererComponent::operator=(RendererComponent&& obj)
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


RendererComponent& RendererComponent::operator=(const RendererComponent& obj)
{
  mRenderer = obj.mRenderer;
  mRenderObj = obj.mRenderObj;
  mMaterial = obj.mMaterial;
  mMeshDescriptor = obj.mMeshDescriptor;
  return (*this);
}
} // Recluse