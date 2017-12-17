// Copyright (c) 2017 Recluse Project. All rights reserved.

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


class MeshData;
class MeshDescriptor;
class SkinnedMesh;
class Material;
class DescriptorSet;
class VulkanRHI;


// A Render object is the forfront of rendering an object on the screen.
// This object is read and used by the renderer to determine object
// rendering. It is a container that references render groups. Keep in mind,
// This render object defines models set in one model space, which all share the 
// same mesh descriptor! (ex. submeshes.)
//
// NOTE(): All mesh data in this render object must share the same mesh descriptor and material, otherwise
// create multiple render objects to define meshes with separate mesh descriptors and/or materials!
class RenderObject {
public:
  // The Mesh descriptor, used to define the renderobject in 3D space.
  MeshDescriptor*         MeshDescriptorId;

  // The material of this Render Object.
  Material*               MaterialId;

  // Number of instances to draw meshdata within the render object.
  // Typically set to 1 (default is 1.)
  u32                     Instances;

  // Is this object renderable? If not, the renderer will ignore it.
  b8                      Renderable;

  RenderObject(MeshDescriptor* mesh = nullptr, 
                Material* material = nullptr);

  ~RenderObject();

  // Initialize the descriptor set within this render group. Does not need to have mesh data for this 
  // to be initialized!
  void                    Initialize();

  // Clean up the descriptor for this render object.
  void                    CleanUp();

  // Updates the descriptor set that is not the current index of this
  // render object. 
  void                    Update();

  // The currently used descriptor set.
  DescriptorSet*          CurrMeshSet() { return mMeshSets[mCurrIdx]; }
  DescriptorSet*          CurrMaterialSet() { return mMaterialSets[mCurrIdx]; }
  DescriptorSet*          CurrBoneSet() { return m_BonesSets[mCurrIdx]; }

  size_t                  Size() const { return mMeshGroup.size(); }

  // The number of descriptor sets that buffer this render object. There are normally
  // 2, since these are used to double buffer when one of the indices updates.
  size_t                  NumOfDescriptorSets() const { return 2; }

  // Current index of this render object's descriptor sets.
  size_t                  CurrIdx() const { return mCurrIdx; }

  void                    SwapDescriptorSet() { mCurrIdx = mCurrIdx == 0 ? 1 : 0; }

  void                    Resize(size_t newSize) { mMeshGroup.resize(newSize); }
  void                    Add(size_t idx, MeshData* data) { mMeshGroup[idx] = data; }
  void                    PushBack(MeshData* data) { mMeshGroup.push_back(data); }

  MeshData*               Get(size_t idx) { return mMeshGroup[idx]; }
  MeshData*               operator[](size_t idx) { return Get(idx); }

protected:  

 void                     UpdateDescriptorSets(size_t idx);

private:
  // The mesh group to render with the following description and material ids.
  std::vector<MeshData*>  mMeshGroup;

  // The actual descriptor set used.
  DescriptorSet*          mMeshSets           [2];
  DescriptorSet*          mMaterialSets       [2];
  DescriptorSet*          m_BonesSets         [2];
  size_t                  mCurrIdx;
  VulkanRHI*              mRhi;
  friend class Renderer;
};
} // Recluse