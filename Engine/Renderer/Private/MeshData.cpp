// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshData.hpp"
#include "Vertex.hpp"

namespace Recluse {


MeshData::MeshData()
 : mRhi(nullptr)
{

}


MeshData::~MeshData()
{
}


void MeshData::Initialize(size_t elementCount, void* data, MeshData::VertexType sizeType, size_t indexCount, void* indices)
{
  size_t size = 0;
  switch (sizeType) {
    case STATIC:
      size = sizeof(StaticVertex); break;
    case SKINNED:
      size = sizeof(SkinnedVertex); break;
    case QUAD:
      size = sizeof(QuadVertex); break;
    default:
      size = 0; break;
  }

  m_vertexBuffer.Initialize(mRhi, elementCount, size, data);

  if (indexCount) {
    m_indexBuffer.Initialize(mRhi, indexCount, sizeof(u32), indices);
  }
}


void MeshData::CleanUp()
{
  m_vertexBuffer.CleanUp();
  m_indexBuffer.CleanUp();
}
} // Recluse