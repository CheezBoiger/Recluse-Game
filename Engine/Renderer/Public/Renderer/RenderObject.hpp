// Copyright (c) 2017 Recluse Project. All rights reserved.

#include "Core/Types.hpp"


namespace Recluse {


class Mesh;
class SkinnedMesh;
class Material;
class DescriptorSet;
class VulkanRHI;

class RenderObject {
public:
  RenderObject(Mesh* mesh, Material* material);
  ~RenderObject();

  void              Initialize();
  void              CleanUp();
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