// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Component.hpp"

#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/Mesh.hpp"
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
  static const I32 kNoMorphIndex = -1;
  AbstractRendererComponent();

  virtual void serialize(IArchive& archive) override { }
  virtual void deserialize(IArchive& archive) override { }

  virtual void onEnable() override;

  void enableShadow(B32 enable);
  void enableStatic(B32 enable);
  void forceForward(B32 enable);
  void enableMorphTargets(B32 enable);
  void setTransparent(B32 enable);
  virtual void enableSkin(B32 enable) { }
  void enableDebug(B32 enable);

  B32 isDirty() const { return m_bDirty; }
  B32 isTransparentEnabled() const;
  B32 isShadowEnabled() const;
  virtual B32 isSkinned() const { return false; }
  virtual MeshDescriptor* getMeshDescriptor(U32 idx = 0) { return nullptr; }
  virtual JointDescriptor* getJointDescriptor(U32 idx = 0) { return nullptr; }


  // These bits corresponds to RenderCommand debug bits. See Renderer/RenderCmd.hpp for DebugConfigBits.
  void setDebugBits(B32 bits);
  void unsetDebugBits(B32 bits);


  void signalClean() { m_bDirty = false; }
  
  virtual void addMesh(Mesh* mesh, U32 idx = Mesh::kMeshUnknownValue) { 
    if (idx == Mesh::kMeshUnknownValue) m_meshes.push_back(mesh);
    else m_meshes[idx] = mesh; 
  }
  
  virtual void clearMeshes() { m_meshes.clear(); }
  Mesh* getMesh(size_t idx) { return m_meshes[idx]; }
  U32 getMeshCount() const { return static_cast<U32>(m_meshes.size()); }
  inline B32 allowAutoLod() const { return m_allowAutoLod; }
  void enableAutoLod(B32 enable) { m_allowAutoLod = enable; }


  R32 getCurrentLod() const { return m_currLod; }
  U32 getMorphIndex0() const { return m_morphIndex0; }
  U32 getMorphIndex1() const { return m_morphIndex1; }

  void setMorphIndex0(U32 idx) { m_morphIndex0 = idx; }
  void setMorphIndex1(U32 idx) { m_morphIndex1 = idx; }

  virtual void setAnimationHandler(AnimHandle* anim) { m_pAnimHandle = anim; }
  virtual AnimHandle* getAnimHandle() { return m_pAnimHandle; }

  virtual void setLodBias(R32 bias, U32 meshIdx = 0) { (void)(meshIdx); m_currLod = bias; }
  virtual R32 getLodBias(U32 meshIdx = 0) const { (void)meshIdx; return m_currLod; }

  virtual void resize(U32 sz) { m_meshes.resize(sz); };

protected:

  virtual void onInitialize(GameObject* owner) override { }
  virtual void onCleanUp() override { }
  virtual void update() override { }
  void triggerDirty() { m_bDirty = true; }
  void updateLod(Transform* meshTransform);

  std::vector<Mesh*> m_meshes;
  B32 m_bDirty;
  U32 m_configs;
  B32 m_debugConfigs;
  B32 m_allowAutoLod;
  R32 m_currLod;
  I32 m_morphIndex0; // Morph index for binding.
  I32 m_morphIndex1; // Morph index for binding.
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

  virtual void enableSkin(B32 enable) override;
  MeshDescriptor* getMeshDescriptor(U32 idx = 0) override { return getSingleMeshDescriptor(); }
  MeshDescriptor* getSingleMeshDescriptor() { return m_meshDescriptor; }
  virtual JointDescriptor* getJointDescriptor(U32 idx = 0) override { return nullptr; }

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
  virtual B32 isSkinned() const override { return true; }
  virtual JointDescriptor* getJointDescriptor(U32 idx = 0) override { return getSingleJointDescriptor(); }
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

  virtual void addMesh(Mesh* mesh, U32 idx = Mesh::kMeshUnknownValue) override;
  virtual void clearMeshes() override;

  void assignMeshParent(U32 meshIdx, U32 parentIdx) { m_perMeshDescriptors[meshIdx].parentId = parentIdx; }

  virtual void resize(U32 sz) override { AbstractRendererComponent::resize(sz); 
                                         m_perMeshDescriptors.resize(sz); }

  virtual void setLodBias(R32 bias, U32 meshIdx = 0) override;
  virtual R32 getLodBias(U32 meshIdx = 0) const override;

protected:
  struct MeshNode {
    MeshDescriptor* _pMeshDescriptor;
    U32 parentId;
  };
  std::vector<MeshNode> m_perMeshDescriptors;
};
} // Recluse