// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "MeshComponent.hpp"
#include "GameObject.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Renderer.hpp"


namespace Recluse {


void Mesh::Initialize(size_t elementCount, void* data, MeshData::VertexType type,size_t indexCount, 
  void* indices, const Vector3& min, const Vector3& max)
{
  m_pData = gRenderer().CreateMeshData();
  m_pData->Initialize(elementCount, data, type, indexCount, indices);
  m_pData->SetMin(min);
  m_pData->SetMax(max);
}


void Mesh::CleanUp()
{
  gRenderer().FreeMeshData(m_pData);
  m_pData = nullptr;
}
} // Recluse