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
  static const i32 kNoMorphIndex = -1;

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
  void                      EnableStatic(b32 enable);
  void                      ForceForward(b32 enable);
  void                      EnableMorphTargets(b32 enable);
  void                      SetTransparent(b32 enable);
  void                      EnableSkin(b32 enable);
  void                      EnableDebug(b32 enable);

  // These bits corresponds to RenderCommand debug bits. See Renderer/RenderCmd.hpp for DebugConfigBits.
  void                      SetDebugBits(b32 bits);
  void                      UnSetDebugBits(b32 bits);

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
  inline b32                AllowLod() const { return m_allowLod; }
  void                      EnableLod(b32 enable) { m_allowLod = enable; }

  u32                       GetCurrentLod() const { return m_currLod; }
  u32                       GetMorphIndex0() const { return m_morphIndex0; }
  u32                       GetMorphIndex1() const { return m_morphIndex1; }

  void                      SetMorphIndex0(u32 idx) { m_morphIndex0 = idx; }
  void                      SetMorphIndex1(u32 idx) { m_morphIndex1 = idx; }

  virtual void              SetAnimationHandler(AnimHandle* anim) { m_pAnimHandle = anim; }
  virtual AnimHandle*       GetAnimHandle() { return m_pAnimHandle; }

protected:
  virtual void              OnInitialize(GameObject* owner) override;
  virtual void              OnCleanUp() override;
  virtual void              Update() override;

  void                      TriggerDirty() { m_bDirty = true; }

private:
  void                      UpdateLod(Transform* meshTransform);

protected:
  AnimHandle*               m_pAnimHandle;
  MeshDescriptor*           m_meshDescriptor;
  std::vector<Mesh*>        m_meshes;
  b32                       m_bDirty;
  u32                       m_configs;
  b32                       m_debugConfigs;
  b32                       m_allowLod;
  u32                       m_currLod;
  i32                       m_morphIndex0; // Morph index for binding.
  i32                       m_morphIndex1; // Morph index for binding.
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

protected:
  JointDescriptor*          m_pJointDescriptor;  
};
} // Recluse