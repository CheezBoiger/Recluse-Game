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
  virtual void              Update() override;

  void                      SetBaseRough(r32 rough) { mMaterial->Data()->_BaseRough = rough; }
  void                      SetBaseMetal(r32 metal) { mMaterial->Data()->_BaseMetal = metal; }
  void                      SetBaseEmissive(r32 emissive) { mMaterial->Data()->_BaseEmissive = emissive; }

  void                      SetAlbedo(Texture2D* texture) { mMaterial->SetAlbedo(texture); }
  void                      SetNormal(Texture2D* texture) { mMaterial->SetNormal(texture); }
  void                      SetRoughness(Texture2D* texture) { mMaterial->SetRoughness(texture); }
  void                      SetMetallic(Texture2D* texture) { mMaterial->SetMetallic(texture); }
  void                      SetAo(Texture2D* texture) { mMaterial->SetAo(texture); }
  void                      SetBaseColor(Vector4 color) { mMaterial->Data()->_Color = color; }  

  void                      EnableAlbedo(b8 enable) { mMaterial->Data()->_HasAlbedo = enable; }
  void                      EnableNormal(b8 enable) { mMaterial->Data()->_HasNormal = enable; }
  void                      EnableRoughness(b8 enable) { mMaterial->Data()->_HasRoughness = enable; }
  void                      EnableMetallic(b8 enable) { mMaterial->Data()->_HasMetallic = enable; }
  void                      EnableEmissive(b8 enable) { mMaterial->Data()->_HasEmissive = enable; }
  void                      EnableAo(b8 enable) { mMaterial->Data()->_HasAO = enable; }

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