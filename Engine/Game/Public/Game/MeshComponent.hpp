// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Game/Component.hpp"

#include "Renderer/MeshData.hpp"


namespace Recluse {


class MeshData;


// A Single instance of a mesh stored in gpu memory.
class Mesh {
public:
  Mesh() : m_pData(nullptr)  { }

  // Initialize the mesh object.
  void            Initialize(size_t elementCount, void* data, MeshData::VertexType type, 
                    size_t indexCount, void* indices, const Vector3& min = Vector3(), const Vector3& max = Vector3());

  // Clean up the mesh object when no longer being used.
  void            CleanUp();

  MeshData*       Native() { return m_pData; }

private:
  MeshData*       m_pData;
};


// Mesh Component holds a reference to mesh objects that are loaded.
class MeshComponent : public Component {
  RCOMPONENT(MeshComponent);
protected:
  virtual void OnInitialize(GameObject* owner) override { }
  virtual void OnCleanUp() override { }
public:

  void SetMeshRef(Mesh* pData) { m_pMeshRef = pData; }
  
  Mesh* MeshRef() { return m_pMeshRef; }

private:
  Mesh*       m_pMeshRef;
};
} // Recluse