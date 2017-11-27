// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "Game/Component.hpp"

namespace Recluse {


class Material;
class SkinnedMeshDescriptor;
class RenderObject;
class Renderer;

// Mesh component which also uses skinned types for animation and 
// whatnot.
class SkinnedMeshComponent : public Component {
  RCOMPONENT(SkinnedMeshComponent)
public:

  SkinnedMeshComponent();
  SkinnedMeshComponent(const SkinnedMeshComponent&);
  SkinnedMeshComponent(SkinnedMeshComponent&&);
  ~SkinnedMeshComponent();

  SkinnedMeshComponent& operator=(const SkinnedMeshComponent&);
  SkinnedMeshComponent& operator=(SkinnedMeshComponent&&);

  void                  Initialize(Renderer* renderer, Material* material, SkinnedMeshDescriptor* descriptor);
  void                  CleanUp();

private:
  Renderer*               mRenderer;
  Material*               mMaterial;
  RenderObject*           mRenderObj;
  SkinnedMeshDescriptor*  mMesh;
};
} // Recluse 