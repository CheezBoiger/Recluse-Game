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

class AbstractRendererComponent : public Component {
  RCOMPONENT(AbstractRendererComponent)
public:
  static const i32 kNoMorphIndex = -1;
  AbstractRendererComponent();

  virtual void serialize(IArchive& archive) override { }
  virtual void deserialize(IArchive& archive) override { }

  virtual void onEnable() override;

  void enableShadow(b32 enable);
  void enableStatic(b32 enable);
  void forceForward(b32 enable);
  void enableMorphTargets(b32 enable);
  void setTransparent(b32 enable);
  virtual void enableSkin(b32 enable) { }
  void enableDebug(b32 enable);

  b32 isDirty() const { return m_bDirty; }
  b32 isTransparentEnabled() const;
  b32 isShadowEnabled() const;
  virtual b32 isSkinned() const { return false; }
  virtual MeshDescriptor* getMeshDescriptor(u32 idx = 0) { return nullptr; }
  virtual JointDescriptor* getJointDescriptor(u32 idx = 0) { return nullptr; }


  // These bits corresponds to RenderCommand debug bits. See Renderer/RenderCmd.hpp for DebugConfigBits.
  void setDebugBits(b32 bits);
  void unsetDebugBits(b32 bits);


  void signalClean() { m_bDirty = false; }
  virtual void addMesh(Mesh* mesh) { m_meshes.push_back(mesh); }
  virtual void clearMeshes() { m_meshes.clear(); }
  Mesh* getMesh(size_t idx) { return m_meshes[idx]; }
  u32 getMeshCount() const { return static_cast<u32>(m_meshes.size()); }
  inline b32 allowLod() const { return m_allowLod; }
  void enableLod(b32 enable) { m_allowLod = enable; }


  u32 getCurrentLod() const { return m_currLod; }
  u32 getMorphIndex0() const { return m_morphIndex0; }
  u32 getMorphIndex1() const { return m_morphIndex1; }

  void setMorphIndex0(u32 idx) { m_morphIndex0 = idx; }
  void setMorphIndex1(u32 idx) { m_morphIndex1 = idx; }

  virtual void setAnimationHandler(AnimHandle* anim) { m_pAnimHandle = anim; }
  virtual AnimHandle* getAnimHandle() { return m_pAnimHandle; }

protected:

  virtual void onInitialize(GameObject* owner) override { }
  virtual void onCleanUp() override { }
  virtual void update() override { }
  void triggerDirty() { m_bDirty = true; }
  void updateLod(Transform* meshTransform);

  std::vector<Mesh*> m_meshes;
  b32 m_bDirty;
  u32 m_configs;
  b32 m_debugConfigs;
  b32 m_allowLod;
  u32 m_currLod;
  i32 m_morphIndex0; // Morph index for binding.
  i32 m_morphIndex1; // Morph index for binding.
  AnimHandle* m_pAnimHandle;

};

// Renderer Component, which holds static mesh object info for rendering
// data. Supports multiple meshes within one renderer component.
class RendererComponent : public AbstractRendererComponent {
public:

  virtual ~RendererComponent() { }
  RendererComponent();
  RendererComponent(const RendererComponent& m);
  RendererComponent(RendererComponent&& m);
  RendererComponent& operator=(RendererComponent&& obj);
  RendererComponent& operator=(const RendererComponent& obj);

  virtual void serialize(IArchive& archive) override { }
  virtual void deserialize(IArchive& archive) override { }

  virtual void enableSkin(b32 enable) override;
  MeshDescriptor* getMeshDescriptor(u32 idx = 0) override { return getSingleMeshDescriptor(); }
  MeshDescriptor* getSingleMeshDescriptor() { return m_meshDescriptor; }
  virtual JointDescriptor* getJointDescriptor(u32 idx = 0) override { return nullptr; }

protected:
  virtual void onInitialize(GameObject* owner) override;
  virtual void onCleanUp() override;
  virtual void update() override;
protected:
  MeshDescriptor* m_meshDescriptor;

};


// Renderer component that holds skinned mesh objects, for animation and 
// whatnot. Supports multiple skinned mesh objects that share the same animation.
class SkinnedRendererComponent : public RendererComponent {
public:
  SkinnedRendererComponent();

  virtual void onInitialize(GameObject* owner) override;
  virtual void onCleanUp() override;
  virtual void update() override;
  virtual void serialize(IArchive& a) override { }
  virtual void deserialize(IArchive& a) override { }
  virtual b32 isSkinned() const override { return true; }
  virtual JointDescriptor* getJointDescriptor(u32 idx = 0) override { return getSingleJointDescriptor(); }
  JointDescriptor* getSingleJointDescriptor() { return m_pJointDescriptor; }
  
protected:
  JointDescriptor* m_pJointDescriptor;
};


// Renderer component meant to store multiple mesh objects, with their own descriptors.
class BatchRendererComponent : public AbstractRendererComponent {
public:

  virtual void onInitialize(GameObject* owner) override;
  virtual void onCleanUp() override;
  virtual void update() override;
  virtual void serialize(IArchive& a) override { }
  virtual void deserialize(IArchive& a) override { }

  virtual void addMesh(Mesh* mesh) override;
  virtual void clearMeshes() override;

protected:
  std::vector<MeshDescriptor*> m_perMeshDescriptors;
};
} // Recluse