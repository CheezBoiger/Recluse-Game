// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Plane.hpp"
#include "Core/Math/Matrix4.hpp"

namespace Recluse {


struct AABB;

// View frustum object.
struct ViewFrustum {
  static u32    PLEFT;
  static u32    PRIGHT;
  static u32    PTOP;
  static u32    PBOTTOM;
  static u32    PNEAR;
  static u32    PFAR;

  ViewFrustum() { }


  Plane& operator[](size_t idx) { return _planes[idx]; }

  // Planes of the camera. Ordered as so:
  // idx        plane
  // 0          left
  // 1          right
  // 2          bottom
  // 3          top
  // 4          near
  // 5          far
  Plane       _planes      [6];
  
  // Function to calculate view frustum. Takes in 
  // a matrix = M * P (View-Projection) matrix to calculate
  // the frustum.
  void        Update(Matrix4& vp);

  b32          Intersect(const AABB* aabb);

  // Check if AABB object is inside this frustum.
  b32          InsideFrustum(const AABB* aabb);
};
} // Recluse