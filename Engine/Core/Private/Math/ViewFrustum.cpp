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


void ViewFrustum::Update(Matrix4& vp)
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
} // Recluse