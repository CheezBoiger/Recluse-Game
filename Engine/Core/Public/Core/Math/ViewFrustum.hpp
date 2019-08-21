// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Plane.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/AABB.hpp"

namespace Recluse {


// View frustum object.
struct ViewFrustum {
  static U32    PLEFT;
  static U32    PRIGHT;
  static U32    PTOP;
  static U32    PBOTTOM;
  static U32    PNEAR;
  static U32    PFAR;

  enum Result {
    Result_Outside,
    Result_Inside,
    Result_Intersect
  };

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
  // a matrix = M * P (View-getProjection) matrix to calculate
  // the frustum.
  void        update(Matrix4& vp);

  Result          intersect(const AABB& aabb) const;

  // Check if AABB object is inside this frustum.
  B32          insideFrustum(const AABB& aabb) { return false; }
};
} // Recluse