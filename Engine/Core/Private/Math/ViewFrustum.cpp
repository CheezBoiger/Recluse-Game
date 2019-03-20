// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/ViewFrustum.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"

namespace Recluse {


u32 ViewFrustum::PLEFT = 0;
u32 ViewFrustum::PRIGHT = 1;
u32 ViewFrustum::PBOTTOM = 2;
u32 ViewFrustum::PTOP = 3;
u32 ViewFrustum::PNEAR = 4;
u32 ViewFrustum::PFAR = 5;


void ViewFrustum::update(Matrix4& vp)
{ 
  _planes[PLEFT] = Plane(  -vp[0][0] - vp[0][3],
                           -vp[1][0] - vp[1][3],
                           -vp[2][0] - vp[2][3],
                           -vp[3][0] - vp[3][3]);

  _planes[PRIGHT] = Plane(  vp[0][0] - vp[0][3],
                            vp[1][0] - vp[1][3],
                            vp[2][0] - vp[2][3],
                            vp[3][0] - vp[3][3]);

  _planes[PBOTTOM] = Plane( vp[0][1] - vp[0][3],
                            vp[1][1] - vp[1][3],
                            vp[2][1] - vp[2][3],
                            vp[3][1] - vp[3][3]);

  _planes[PTOP] = Plane(   -vp[0][1] - vp[0][3],
                           -vp[1][1] - vp[1][3],
                           -vp[2][1] - vp[2][3],
                           -vp[3][1] - vp[3][3]); 

  _planes[PNEAR] = Plane(  -vp[0][2] - vp[0][3],
                           -vp[1][2] - vp[1][3],
                           -vp[2][2] - vp[2][3],
                           -vp[3][2] - vp[3][3]);

  _planes[PFAR] = Plane(    vp[0][2] - vp[0][3],
                            vp[1][2] - vp[1][3],
                            vp[2][2] - vp[2][3],
                            vp[3][2] - vp[3][3]);
}


ViewFrustum::Result ViewFrustum::intersect(const AABB& aabb) const
{
  // TODO(): Needs fixing as it is not properly culling.

  ViewFrustum::Result result = Result_Inside;
  Vector3 vmin, vmax;
  for (u32 i = 0; i < 6; ++i) {
    Vector3 normal = Vector3(_planes[i].x, _planes[i].y, _planes[i].z);
    // x axis.
    if (_planes[i].x > 0) {
      vmin.x = aabb.min.x;
      vmax.x = aabb.max.x;  
    } else {
      vmin.x = aabb.max.x;
      vmax.x = aabb.min.x;
    }

    // y axis.
    if (_planes[i].y > 0) {
      vmin.y = aabb.min.y;
      vmax.y = aabb.max.y;
    } else {
      vmin.y = aabb.max.y;
      vmax.y = aabb.min.y;
    }

    // z axis.
    if (_planes[i].z > 0) {
      vmin.z = aabb.min.z;
      vmax.z = aabb.max.z;
    } else {
      vmin.z = aabb.max.z;
      vmax.z = aabb.min.z;
    }
    if (normal.dot(vmin) + _planes[i].w > 0.0f) {
      result = Result_Outside; 
      break;
    }
    if (normal.dot(vmax) + _planes[i].w >= 0.0f) {
      result = Result_Intersect;
      break;
    }
  }
  return result;
}
} // Recluse