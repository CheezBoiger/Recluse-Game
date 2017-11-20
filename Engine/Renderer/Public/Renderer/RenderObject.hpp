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
  RenderObject(MeshDescriptor* mesh = nullptr, 
                Material* material = nullptr);

  ~RenderObject();

  // Initialize the descriptor set within this render group. Does not need to have mesh data for this 
  // to be initialized!
  void                    Initialize();

  // Clean up the descriptor for this render object.
  void                    CleanUp();

  // Updates the descriptor set of this object. Calling this will require 
  // rebuilding of the commandbuffers in the renderer (calling Build() from 
  // Renderer.)
  void                    Update();

  // The descriptor set initialized in this renderobject.
  DescriptorSet*          Set() { return mDescriptorSet; }

  // The Mesh descriptor, used to define the renderobject in 3D space.
  MeshDescriptor*         meshDescriptorId;

  // The material of this Render Object.
  Material*               materialId;

  // Does this render object define a skinned mesh descriptor?
  b8                      skinned;

  size_t                  Size() const { return mMeshGroup.size(); }

  void                    Resize(size_t newSize) { mMeshGroup.resize(newSize); }
  void                    Add(size_t idx, MeshData* data) { mMeshGroup[idx] = data; }
  void                    PushBack(MeshData* data) { mMeshGroup.push_back(data); }

  MeshData*               Get(size_t idx) { return mMeshGroup[idx]; }
  MeshData*               operator[](size_t idx) { return Get(idx); }
  
private:
  void                    UpdateDescriptorSet(b8 includeBufferUpdate);

  // The mesh group to render with the following description and material ids.
  std::vector<MeshData*>  mMeshGroup;

  // The actual descriptor set used.
  DescriptorSet*          mDescriptorSet;

  VulkanRHI*              mRhi;
  friend class Renderer;
};
} // Recluse