// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshData.hpp"
#include "Vertex.hpp"

#include "MaterialDescriptor.hpp"

#include <algorithm>

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


void MeshData::SortPrimitives(MeshData::SortType type)
{
  switch (type) {
    case SortType::TRANSPARENCY_LAST:
    {
      std::vector<Primitive> transparencies;
      std::vector<Primitive> opaques;
      for ( Primitive& primitive : m_primitives ) { 
        if ( primitive._pMat->Transparent() ) {
          transparencies.push_back(primitive);
        } else {
          opaques.push_back(primitive);
        }
      }
      size_t idx = 0;
      for ( Primitive& primitive : opaques ) {
        m_primitives[idx++] = primitive;
      }
      for ( Primitive& primitive : transparencies ) {
        m_primitives[idx++] = primitive;
      }
    } break;
    case SortType::TRANSPARENCY_FIRST:
    {
      std::vector<Primitive> transparencies;
      std::vector<Primitive> opaques;
      for (Primitive& primitive : m_primitives) {
        if (primitive._pMat->Transparent()) {
          transparencies.push_back(primitive);
        }
        else {
          opaques.push_back(primitive);
        }
      }
      size_t idx = 0;
      for (Primitive& primitive : transparencies) {
        m_primitives[idx++] = primitive;
      }
      for (Primitive& primitive : opaques) {
        m_primitives[idx++] = primitive;
      }
    } break;
    default: break;
  }
}
} // Recluse