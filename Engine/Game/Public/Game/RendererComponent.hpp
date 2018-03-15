// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
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

  // Reconfigure this renderer component, this will signal the renderer to 
  // update the cmd buffer, and re construct it's scene. It must be done in order
  // to add new textures. Recommended you do not call this function every frame, as it does
  // hinder rendering performance.
  void                      ReConfigure();
  void                      Enable(b8 enable);
  void                      EnableShadow(b8 enable);

  b8                        Enabled() const;
  b8                        Dirty() const { return m_Dirty; }
  b8                        ShadowEnabled() const;
  RenderObject*             RenderObj() { return mRenderObj; }
  MeshDescriptor*           GetDescriptor() { return mMeshDescriptor; }

  void                      SignalClean() { m_Dirty = false; }

protected:
  void                      TriggerDirty() { m_Dirty = true; }
  RenderObject*             mRenderObj;
  MeshDescriptor*           mMeshDescriptor;
  b8                        m_Dirty;
};


// Renderer component that holds a skinned mesh object, for animation and 
// whatnot.
class SkinnedRendererComponent : public RendererComponent {
public:
  
  virtual void OnInitialize(GameObject* owner) override;
  virtual void OnCleanUp() override;

protected:
};
} // Recluse