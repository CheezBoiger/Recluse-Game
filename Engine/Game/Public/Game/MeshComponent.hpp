// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Component.hpp"

#include "Renderer/Material.hpp"

namespace Recluse {


class RenderObject;
class MeshDescriptor;
class Renderer;
class Material;
class Mesh;


class MeshComponent : public Component {
  RCOMPONENT(MeshComponent)
public:
  MeshComponent() 
    : mRenderer(nullptr)
    , mMaterial(nullptr)
    , mRenderObj(nullptr)
    , mMeshDescriptor(nullptr) { }

  MeshComponent(const MeshComponent& m)
    : mRenderer(m.mRenderer)
    , mMaterial(m.mMaterial)
    , mMeshDescriptor(m.mMeshDescriptor)
    , mRenderObj(m.mRenderObj) { }

  MeshComponent(MeshComponent&& m)
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

  MeshComponent& operator=(MeshComponent&& obj) {
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

  MeshComponent& operator=(const MeshComponent& obj) {
    mRenderer = obj.mRenderer;
    mRenderObj = obj.mRenderObj;
    mMaterial = obj.mMaterial;
    mMeshDescriptor = obj.mMeshDescriptor;
  }

  void            Initialize(Renderer* renderer, const MeshDescriptor* meshDesc, const Material* mat);
  void            CleanUp();

  RenderObject*   RenderObj() { return mRenderObj; }

private:
  Renderer*       mRenderer;
  Material*       mMaterial;
  RenderObject*   mRenderObj;
  MeshDescriptor* mMeshDescriptor;
};
} // Recluse