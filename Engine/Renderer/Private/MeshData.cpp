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


void MeshData::Initialize(MeshLod lod, size_t elementCount, void* data, MeshData::VertexType sizeType, size_t indexCount, void* indices)
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

  m_vertexBuffersLod[lod].Initialize(mRhi, elementCount, size, data);

  if (indexCount) {
    m_indexBuffersLod[lod].Initialize(mRhi, indexCount, sizeof(u32), indices);
  }
}


void MeshData::CleanUp()
{
  for (size_t i = 0; i < kMaxLodMeshCount; ++i) {
    m_vertexBuffersLod[i].CleanUp();
    m_indexBuffersLod[i].CleanUp();
  }
}
} // Recluse