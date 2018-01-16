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
class RendererComponent : public Component {
  RCOMPONENT(RendererComponent)
public:
  virtual ~RendererComponent() { }
  RendererComponent();
  RendererComponent(const RendererComponent& m);
  RendererComponent(RendererComponent&& m);
  RendererComponent& operator=(RendererComponent&& obj);
  RendererComponent& operator=(const RendererComponent& obj);

  virtual void              OnInitialize(GameObject* owner) override;
  virtual void              OnCleanUp() override;
  virtual void              Serialize(IArchive& archive) override { }
  virtual void              Deserialize(IArchive& archive) override { }

  RenderObject*             RenderObj() { return mRenderObj; }
  MaterialDescriptor*       GetMaterial() { return mMaterial; }
  MeshDescriptor*           GetDescriptor() { return mMeshDescriptor; }

protected:
  MaterialDescriptor*       mMaterial;
  RenderObject*             mRenderObj;
  MeshDescriptor*           mMeshDescriptor;
};


// Renderer component that holds a skinned mesh object, for animation and 
// whatnot.
class SkinnedRendererComponent : public RendererComponent {
public:
  
};
} // Recluse