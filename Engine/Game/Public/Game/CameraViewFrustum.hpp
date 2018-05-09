// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Plane.hpp"

namespace Recluse {


class Camera;
class Transform;
class AABB;

typedef struct CCamViewFrustum CFrustum;

struct CCamViewFrustum {
  static u32    PLEFT;
  static u32    PRIGHT;
  static u32    PTOP;
  static u32    PBOTTOM;
  static u32    PNEAR;
  static u32    PFAR;

private:
  // camera that this view frustum is associated with.
  Camera*     m_pCamera;
  r32         m_Nh;
  r32         m_Nw;
  r32         m_Fh;
  r32         m_Fw;

public:
  CCamViewFrustum()
    : m_pCamera(nullptr) { }

  // Planes of the camera. Ordered as so:
  // idx        plane
  // 0          left
  // 1          right
  // 2          top
  // 3          bottom
  // 4          near
  // 5          far
  Plane       _Planes      [6];
  
  // TODO(): Update() function to calculate camera view frustum
  // which will be used to send to global material.
  void        Update(Transform* transform);


  b32          Intersect(const AABB* aabb);

  // Check if AABB object is inside this frustum.
  b32          InsideFrustum(const AABB* aabb);

  void        SetCamera(Camera* camera);
  Camera*     GetCamera() const { return m_pCamera; }

protected:
  void        ConfigureFrustum();
  
};
} // Recluse