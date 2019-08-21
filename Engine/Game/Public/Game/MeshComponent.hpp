// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Game/Component.hpp"
#include "Core/Math/AABB.hpp"
#include "Renderer/Mesh.hpp"

#include "Animation/Skeleton.hpp"


namespace Recluse {


// Mesh Component holds a reference to mesh objects that are loaded.
class MeshComponent : public Component {
  RCOMPONENT(MeshComponent);
protected:
  virtual void    onInitialize(GameObject* owner) override;
  virtual void    onCleanUp() override;
public:
  MeshComponent();

  void            SetMeshRef(Mesh* pData) { m_pMeshRef = pData; }
  
  Mesh*           MeshRef() { return m_pMeshRef; }

  // updates this mesh component instance frustum bit cull.
  void            update() override;

  void            EnableCulling(B32 enable) { m_allowCulling = enable; }

  inline B32      AllowCulling() const { return m_allowCulling; }

  // Cull bit set map. Each bit represents a frustum, and when flipped, means this mesh is culled
  // for that given frustum. maximum 32 frustums can be supported.
  B32             GetFrustumCullMap() const { return m_frustumCull; }

  // Set a frustum bit to 1.
  void            SetFrustumCull(B32 mask) { m_frustumCull |= mask; }

  // Unset a frustum bit to 0.
  void            UnsetFrustumCull(B32 mask) { m_frustumCull &= ~mask; }

  // Clear and reset all frustum bits to 0.
  void            ClearFrustumCullBits() { m_frustumCull &= 0; }

private:

  void            UpdateFrustumCullBits();

  Mesh*           m_pMeshRef;
  B32             m_frustumCull;
  B32             m_allowCulling;

};
} // Recluse