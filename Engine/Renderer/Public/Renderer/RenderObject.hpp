// Copyright (c) 2017 Recluse Project. All rights reserved.

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


class MeshData;
class MeshDescriptor;
class SkinnedMesh;
class MaterialDescriptor;
class SkinnedMeshDescriptor;
class DescriptorSet;
class VulkanRHI;


// Primitive information for render object.
struct PrimitiveInfo {
  u32                 _startIdx;
  u32                 _idxCount;
  MaterialDescriptor* _pMaterialDescriptor;
};


// A Render object is the forfront of rendering an object on the screen.
// This object is read and used by the renderer to determine object
// rendering. It is a container that references render groups. Keep in mind,
// This render object defines models set in one model space, which all share the 
// same mesh descriptor! (ex. submeshes -> primitives)
//
// NOTE(): All mesh data in this render object must share the same mesh descriptor and material, otherwise
// create multiple render objects to define meshes with separate mesh descriptors and/or materials!
class RenderObject {
public:
#define SIGNAL_RENDER_OBJECT_UPDATE() m_bNeedsUpdate = true
  // Number of instances to draw meshdata within the render object.
  // Typically set to 1 (default is 1.)
  u32                     _uInstances;

  // Is this object renderable? If not, the renderer will ignore it.
  b32                      _bRenderable;

  // Checks if shadow is enabled for this Command. If false, renderer
  // will opt out from rendering shadows for this render object. No shadow is produced from this object when set
  // false. This can speed up performance.
  b32                      _bEnableShadow;

  RenderObject(uuid64 uuid, MeshDescriptor* mesh = nullptr, 
                MaterialDescriptor* material = nullptr);

  virtual ~RenderObject();

  // Initialize the descriptor set within this render group. Does not need to have mesh data for this 
  // to be initialized!
  virtual void            Initialize();

  // Clean up the descriptor for this render object.
  virtual void            CleanUp();

  // Updates the descriptor set that is not the current index of this
  // render object. 
  virtual void            Update();

  uuid64                  GetUUID() const { return m_uuid; }

  // The currently used descriptor set.
  DescriptorSet*          CurrMeshSet() { return mMeshSets[mCurrIdx]; }
  DescriptorSet*          CurrMaterialSet() { return mMaterialSets[mCurrIdx]; }
  virtual DescriptorSet*  CurrJointSet() { return nullptr; }

  size_t                  Size() const { return mMeshGroup.size(); }

  // The number of descriptor sets that buffer this render object. There are normally
  // 2, since these are used to double buffer when one of the indices updates.
  size_t                  NumOfDescriptorSets() const { return 2; }

  // Current index of this render object's descriptor sets.
  size_t                  CurrIdx() const { return mCurrIdx; }

  void                    SwapDescriptorSet() { mCurrIdx = mCurrIdx == 0 ? 1 : 0; }

  void                    Resize(size_t newSize) { mMeshGroup.resize(newSize); }
  void                    Add(size_t idx, MeshData* data) { mMeshGroup[idx] = data; SIGNAL_RENDER_OBJECT_UPDATE(); }
  void                    PushBack(MeshData* data) { mMeshGroup.push_back(data); SIGNAL_RENDER_OBJECT_UPDATE(); }
  void                    RemoveMesh(size_t idx) { mMeshGroup.erase(mMeshGroup.begin() + idx); SIGNAL_RENDER_OBJECT_UPDATE(); }
  void                    ClearOutMeshGroup() { mMeshGroup.clear(); SIGNAL_RENDER_OBJECT_UPDATE(); }
  void                    SetMeshDescriptor(MeshDescriptor* md) { _pMeshDescId = md; SIGNAL_RENDER_OBJECT_UPDATE(); }
  void                    SetMaterialDescriptor(MaterialDescriptor* md) { _pMaterialDescId = md; SIGNAL_RENDER_OBJECT_UPDATE(); }

  MeshData*               Get(size_t idx) { return mMeshGroup[idx]; }
  MeshData*               operator[](size_t idx) { return Get(idx); }

  MeshDescriptor*         GetMeshDescriptor() { return _pMeshDescId; }
  MaterialDescriptor*     GetMaterialDescriptor() { return _pMaterialDescId; }

protected:  

 virtual void                     UpdateDescriptorSets(size_t idx);

protected:
  // The mesh group to render with the following description and material ids.
  std::vector<MeshData*>  mMeshGroup;

  // The actual descriptor set used.
  DescriptorSet*          mMeshSets           [2];
  DescriptorSet*          mMaterialSets       [2];
  // The Mesh descriptor, used to define the renderobject in 3D space.
  MeshDescriptor*         _pMeshDescId;

  // The material of this Render Object.
  MaterialDescriptor*     _pMaterialDescId;
  size_t                  mCurrIdx;

  // UUID identifer that corresponds to this render object.
  uuid64                  m_uuid;
  VulkanRHI*              mRhi;
  b32                     m_bNeedsUpdate;
  friend class Renderer;
};


// Skinned Render object implementation, which allows updating joint sets.
// This is used for animation purposes.
class SkinnedRenderObject : public RenderObject {
public:
  SkinnedRenderObject(uuid64 uuid, SkinnedMeshDescriptor* smd,
    MaterialDescriptor* md);

  virtual DescriptorSet*  CurrJointSet() override { return m_JointSets[mCurrIdx]; }
  virtual void Initialize() override;
  virtual void CleanUp() override;
  virtual void Update() override;

private:

  void UpdateJointSets(size_t idx);

  DescriptorSet* m_JointSets[2];
};
} // Recluse