// Copyright (c) 2017 Recluse Project. All rights reserved.

#include "Core/Types.hpp"


namespace Recluse {


class Mesh;
class SkinnedMesh;
class Material;
class DescriptorSet;
class VulkanRHI;

// A Render object is the forfront of rendering an object on the screen.
// This object tells the renderer how to draw something, so a material 
// and Mesh object are required.
class RenderObject {
public:
  RenderObject(Mesh* mesh, Material* material);
  ~RenderObject();

  void              Initialize();
  void              CleanUp();

  // Updates the descriptor set of this object. Calling this will require 
  // rebuilding of the commandbuffers in the renderer (calling Build() from 
  // Renderer.)
  void              Update();

  DescriptorSet*    Set() { return mDescriptorSet; }

  Mesh*             meshId;
  Material*         materialId;
  b8                skinned;

public:
  void            UpdateDescriptorSet(b8 includeBufferUpdate);

  DescriptorSet*    mDescriptorSet;
  VulkanRHI*        mRhi;

  friend class Renderer;
};
} // Recluse