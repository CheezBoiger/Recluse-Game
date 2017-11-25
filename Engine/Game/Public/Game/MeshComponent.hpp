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