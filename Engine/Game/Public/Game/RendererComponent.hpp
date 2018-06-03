// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Component.hpp"

#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/MeshData.hpp"
#include <unordered_map>


namespace Recluse {


class MeshDescriptor;
class Renderer;
class MaterialDescriptor;
class MeshComponent;
class MaterialComponent;
class AnimationComponent;
class Mesh;

struct Primitive;


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

  virtual void              Serialize(IArchive& archive) override { }
  virtual void              Deserialize(IArchive& archive) override { }

  void                      OnEnable() override;
  void                      EnableShadow(b32 enable);
  void                      ForceForward(b32 enable);
  void                      SetTransparent(b32 enable);

  b32                       Dirty() const { return m_bDirty; }
  b32                       TransparentEnabled() const;
  b32                       ShadowEnabled() const;
  MeshDescriptor*           GetMeshDescriptor() { return m_meshDescriptor; }

  void                      SignalClean() { m_bDirty = false; }
  inline void                      SetPrimitive(Primitive& primitive) { m_primitives.push_back(primitive); }
  void                      SetMeshComponent(MeshComponent* mesh) { m_meshRef = mesh; }
  virtual void              SetAnimationComponent(AnimationComponent* anim) { }

  inline void                      ClearPrimitives() { m_primitives.clear(); }

protected:
  virtual void              OnInitialize(GameObject* owner) override;
  virtual void              OnCleanUp() override;
  virtual void              Update() override;

  void                      TriggerDirty() { m_bDirty = true; }
  MeshDescriptor*           m_meshDescriptor;
  MeshComponent*            m_meshRef;
  std::vector<Primitive>    m_primitives;
  b32                       m_bDirty;
  u32                       m_configs;
};


// Renderer component that holds a skinned mesh object, for animation and 
// whatnot.
class SkinnedRendererComponent : public RendererComponent {
  RCOMPONENT(SkinnedRendererComponent)
public:

  virtual void OnInitialize(GameObject* owner) override;
  virtual void OnCleanUp() override;
  virtual void Update() override;
  virtual void Serialize(IArchive& a) override { }
  virtual void Deserialize(IArchive& a) override { }

  virtual void SetAnimationComponent(AnimationComponent* anim) { m_pAnimComponent = anim; }

protected:
  AnimationComponent*     m_pAnimComponent;
};
} // Recluse