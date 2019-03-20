// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "MeshData.hpp"
#include "Vertex.hpp"
#include "Renderer.hpp"
#include "MaterialDescriptor.hpp"

#include <algorithm>

namespace Recluse {


MeshData::MeshData()
{

}


MeshData::~MeshData()
{
}


void MeshData::initialize(Renderer* pRenderer, 
  size_t elementCount, void* data, size_t vertexSize, size_t indexCount, void* indices)
{
  
  m_vertexBuffer.initialize(pRenderer->getRHI(), elementCount, vertexSize, data);

  if (indexCount) {
    m_indexBuffer.initialize(pRenderer->getRHI(), indexCount, sizeof(u32), indices);
  }
}


void MeshData::cleanUp(Renderer* pRenderer)
{
  m_vertexBuffer.cleanUp(pRenderer->getRHI());
  m_indexBuffer.cleanUp(pRenderer->getRHI());
}


MorphTarget::MorphTarget()
{
}


MorphTarget::~MorphTarget()
{
}


void MorphTarget::initialize(Renderer* pRenderer, size_t elementCount, void* data, size_t vertexSize)
{
  m_vertexBuffer.initialize(pRenderer->getRHI(), elementCount, vertexSize, data);
}

void MorphTarget::cleanUp(Renderer* pRenderer)
{
  m_vertexBuffer.cleanUp(pRenderer->getRHI());
}
} // Recluse