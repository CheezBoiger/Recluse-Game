// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Component.hpp"

#include "Renderer/MaterialDescriptor.hpp"

namespace Recluse {


class RenderObject;
class MeshDescriptor;
class Renderer;
class MaterialDescriptor;
class Mesh;


// Mesh Component, which holds static mesh object info for rendering
// data.
class MeshComponent : public Component {
  RCOMPONENT(MeshComponent)
public:
  MeshComponent();
  MeshComponent(const MeshComponent& m);
  MeshComponent(MeshComponent&& m);
  MeshComponent& operator=(MeshComponent&& obj);
  MeshComponent& operator=(const MeshComponent& obj);

  void                      Initialize(Renderer* renderer, const MeshDescriptor* meshDesc, const MaterialDescriptor* mat);
  void                      CleanUp();
  void                      Serialize(IArchive& archive) override { }
  void                      Deserialize(IArchive& archive) override { }

  RenderObject*             RenderObj() { return mRenderObj; }
  Renderer*                 GetRenderer() { return mRenderer; }
  MaterialDescriptor*       GetMaterial() { return mMaterial; }
  MeshDescriptor*           GetDescriptor() { return mMeshDescriptor; }

private:
  Renderer*                 mRenderer;
  MaterialDescriptor*       mMaterial;
  RenderObject*             mRenderObj;
  MeshDescriptor*           mMeshDescriptor;
};
} // Recluse