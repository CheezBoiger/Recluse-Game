// Copyright (c) Recluse Project. All rights reserved.
#include "Mesh.hpp"
#include "Core/Exception.hpp"
#include "Vertex.hpp"
#include "Renderer.hpp"
#include "Material.hpp"

namespace Recluse {


void Mesh::initialize(Renderer* pRenderer ,size_t elementCount, void* data, VertexType type, 
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
  m_pMeshData = new MeshData();
  m_pMeshData->initialize(pRenderer, elementCount, data, size, indexCount, indices);
  if (type == VertexType::SKINNED) m_bSkinned = true;
}


void Mesh::cleanUp(Renderer* pRenderer)
{
  m_pMeshData->cleanUp(pRenderer);
  delete m_pMeshData;

  clearMorphTargets(pRenderer);
}

void Mesh::sortPrimitives(Mesh::SortType type)
{
  switch (type) {
    case SortType::TRANSPARENCY_LAST:
    {
      std::vector<Primitive> transparencies;
      std::vector<Primitive> opaques;
      for ( Primitive& primitive : m_primitives ) { 
        if ( primitive._pMat->getNative()->isTransparent() ) {
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
        if (primitive._pMat->getNative()->isTransparent()) {
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


void Mesh::allocateMorphTargetBuffer(size_t newSize)
{
  m_morphTargets.resize(newSize);
  for (size_t i = 0; i < m_morphTargets.size(); ++i) {
    m_morphTargets[i] = nullptr;
  }
}


void Mesh::initializeMorphTarget(Renderer* pRenderer ,size_t idx, size_t elementCount, void* data, size_t vertexSize)
{
  m_morphTargets[idx] = new MorphTarget();
  m_morphTargets[idx]->initialize(pRenderer, elementCount, data, vertexSize);
}


void Mesh::clearMorphTargets(Renderer* pRenderer)
{
  for (size_t i = 0; i < m_morphTargets.size(); ++i) {
    MorphTarget* target = m_morphTargets[i];
    if ( target ) {
      target->cleanUp(pRenderer);
      delete target;
      m_morphTargets[i] = nullptr;
    }
  }
}
} // Recluse 