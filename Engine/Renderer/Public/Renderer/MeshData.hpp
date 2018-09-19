// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Core/Math/AABB.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Utility/Vector.hpp"

namespace Recluse {


class VulkanRHI;
class MaterialDescriptor;
class MeshData;

// Mesh data represents data, in the form of gpu friendly buffers, to which we draw onto the 
// frame. We use mesh data to represent the model we are drawing.
class MeshData {
  static const size_t kMaxLodMeshCount = 4;
public:
  MeshData();
  ~MeshData();

  void            Initialize(size_t elementCount, void* data, size_t vertexSize,
    size_t indexCount = 0, void* indices = nullptr);
  void            CleanUp();

  VertexBuffer*   VertexData() { return &m_vertexBuffer; }
  
  IndexBuffer*    IndexData() { 
    if (m_indexBuffer.IndexCount() > 0) return &m_indexBuffer; 
    else return nullptr; 
  }

  // Optimize the mesh to a certain specification.
  void                    Optimize();

  // Compress mesh data to become optimal by lod.
  void                    CompressByLod(u32 lod);

private:
  VertexBuffer                        m_vertexBuffer;
  IndexBuffer                         m_indexBuffer;
  VulkanRHI*                          mRhi;
  friend class Renderer;
};
} // Recluse