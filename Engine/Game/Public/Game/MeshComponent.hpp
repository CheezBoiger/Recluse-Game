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

  void            Initialize(Renderer* renderer, const MeshDescriptor* meshDesc, const Material* mat);
  void            CleanUp();
  void            Serialize(IArchive& archive) override;
  void            Deserialize(IArchive& archive) override;

  RenderObject*   RenderObj() { return mRenderObj; }
  Renderer*       GetRenderer() { return mRenderer; }
  Material*       GetMaterial() { return mMaterial; }
  MeshDescriptor* GetDescriptor() { return mMeshDescriptor; }

private:
  Renderer*       mRenderer;
  Material*       mMaterial;
  RenderObject*   mRenderObj;
  MeshDescriptor* mMeshDescriptor;
};
} // Recluse