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
class JointDescriptor;
class Mesh;

struct Primitive;
struct AnimHandle;

// Renderer Component, which holds static mesh object info for rendering
// data. Supports multiple meshes within one renderer component.
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
  void                      EnableSkin(b32 enable);

  b32                       Dirty() const { return m_bDirty; }
  b32                       TransparentEnabled() const;
  b32                       ShadowEnabled() const;
  virtual b32               Skinned() const { return false; }
  MeshDescriptor*           GetMeshDescriptor() { return m_meshDescriptor; }
  virtual JointDescriptor*  GetJointDescriptor() { return nullptr; }

  void                      SignalClean() { m_bDirty = false; }
  void                      AddMesh(Mesh* mesh) { m_meshes.push_back(mesh); }
  void                      ClearMeshes() { m_meshes.clear(); }
  Mesh*                     GetMesh(size_t idx) { return m_meshes[idx]; }
  u32                       GetMeshCount() const { return static_cast<u32>(m_meshes.size()); }
  virtual void              SetAnimationHandler(AnimHandle* anim) { }  
  virtual AnimHandle*       GetAnimHandle() { return nullptr; }

protected:
  virtual void              OnInitialize(GameObject* owner) override;
  virtual void              OnCleanUp() override;
  virtual void              Update() override;

  void                      TriggerDirty() { m_bDirty = true; }

  MeshDescriptor*           m_meshDescriptor;
  std::vector<Mesh*>        m_meshes;
  b32                       m_bDirty;
  u32                       m_configs;
};


// Renderer component that holds skinned mesh objects, for animation and 
// whatnot. Supports multiple skinned mesh objects that share the same animation.
class SkinnedRendererComponent : public RendererComponent {
  RCOMPONENT(SkinnedRendererComponent)
public:
  SkinnedRendererComponent();

  virtual void              OnInitialize(GameObject* owner) override;
  virtual void              OnCleanUp() override;
  virtual void              Update() override;
  virtual void              Serialize(IArchive& a) override { }
  virtual void              Deserialize(IArchive& a) override { }
  virtual b32               Skinned() const override { return true; }
  virtual JointDescriptor*  GetJointDescriptor() override { return m_pJointDescriptor; }

  virtual void              SetAnimationHandler(AnimHandle* anim) { m_pAnimHandle = anim; }
  virtual AnimHandle*       GetAnimHandle() { return m_pAnimHandle; }

protected:
  JointDescriptor*          m_pJointDescriptor;  
  AnimHandle*               m_pAnimHandle;
};
} // Recluse