// Copyright (c) Recluse Project. All rights reserved.
#include "Mesh.hpp"
#include "Core/Exception.hpp"
#include "Vertex.hpp"
#include "Renderer.hpp"
#include "Material.hpp"

namespace Recluse {


void Mesh::Initialize(size_t elementCount, void* data, VertexType type, 
  size_t indexCount, void* indices)
{
  R_ASSERT(!m_pMeshData, "Mesh data at specified lod is already initialized.");
  size_t size = 0; 
  switch (type) {
    case STATIC: size = sizeof(StaticVertex); break;
    case SKINNED: size = sizeof(SkinnedVertex); break;
    case QUAD: size = sizeof(QuadVertex); break;
    default: size = sizeof(StaticVertex); break;
  }
  m_pMeshData = gRenderer().CreateMeshData();
  m_pMeshData->Initialize(elementCount, data, size, indexCount, indices);
  if (type == VertexType::SKINNED) m_bSkinned = true;
}


void Mesh::CleanUp()
{
  gRenderer().FreeMeshData( m_pMeshData );
  m_pMeshData = nullptr;
}

void Mesh::SortPrimitives(Mesh::SortType type)
{
  switch (type) {
    case SortType::TRANSPARENCY_LAST:
    {
      std::vector<Primitive> transparencies;
      std::vector<Primitive> opaques;
      for ( Primitive& primitive : m_primitives ) { 
        if ( primitive._pMat->Native()->Transparent() ) {
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
        if (primitive._pMat->Native()->Transparent()) {
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