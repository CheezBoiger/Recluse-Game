// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "MeshComponent.hpp"
#include "GameObject.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Renderer.hpp"


namespace Recluse {


void Mesh::Initialize(size_t elementCount, size_t sizeType, void* data, b32 isStatic, size_t indexCount, void* indices)
{
  m_pData = gRenderer().CreateMeshData();
  m_pData->Initialize(elementCount, sizeType, data, isStatic, indexCount, indices);
}


void Mesh::CleanUp()
{
  gRenderer().FreeMeshData(m_pData);
  m_pData = nullptr;
}
} // Recluse