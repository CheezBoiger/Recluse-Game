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


void MeshData::Initialize(Renderer* pRenderer, 
  size_t elementCount, void* data, size_t vertexSize, size_t indexCount, void* indices)
{
  
  m_vertexBuffer.Initialize(pRenderer->RHI(), elementCount, vertexSize, data);

  if (indexCount) {
    m_indexBuffer.Initialize(pRenderer->RHI(), indexCount, sizeof(u32), indices);
  }
}


void MeshData::CleanUp(Renderer* pRenderer)
{
  m_vertexBuffer.CleanUp(pRenderer->RHI());
  m_indexBuffer.CleanUp(pRenderer->RHI());
}


MorphTarget::MorphTarget()
{
}


MorphTarget::~MorphTarget()
{
}


void MorphTarget::Initialize(Renderer* pRenderer, size_t elementCount, void* data, size_t vertexSize)
{
  m_vertexBuffer.Initialize(pRenderer->RHI(), elementCount, vertexSize, data);
}

void MorphTarget::CleanUp(Renderer* pRenderer)
{
  m_vertexBuffer.CleanUp(pRenderer->RHI());
}
} // Recluse