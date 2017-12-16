// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "CameraViewFrustum.hpp"
#include "Core/Exception.hpp"
#include "Camera.hpp"

namespace Recluse {


u32 CCamViewFrustum::PLEFT = 0;
u32 CCamViewFrustum::PRIGHT = 1;
u32 CCamViewFrustum::PTOP = 2;
u32 CCamViewFrustum::PBOTTOM = 3;
u32 CCamViewFrustum::PNEAR = 4;
u32 CCamViewFrustum::PFAR = 5;


void CCamViewFrustum::SetCamera(Camera* camera)
{
  if (!camera) return;
  m_Camera = camera;
}


void CCamViewFrustum::ConfigureFrustum()
{
  // Might need to figure out this Tang thing...
  r32 Tang = (r32)tanf(Radians(m_Camera->FoV() * 0.5f));
  m_Nh = m_Camera->Near() * Tang;
  m_Nw = m_Nh * m_Camera->Aspect();
  m_Fh = m_Camera->Far() * Tang;
  m_Fw = m_Fh * m_Camera->Aspect();
}


void CCamViewFrustum::Update()
{
  if (!m_Camera) {
    R_DEBUG(rError, "No camera set to calc frustums from!\n");
    return;
  }

  if (!m_Camera->Culling()) return;
  ConfigureFrustum();
  
  Vector3 l = m_Camera->LookDir();
  Vector3 p = m_Camera->Position();
  Vector3 u = m_Camera->Up();

  Vector3 Nc, Fc, X, Y, Z;

  Z = m_Camera->Front().Normalize();
  X = -(u ^ Z).Normalize();
  Y = Z ^ X;
  
  Nc = p - Z * m_Camera->Near();
  Fc = p - Z * m_Camera->Far();

  Vector3 ntl = Nc + Y * m_Nh - X * m_Nw;
  Vector3 ntr = Nc + Y * m_Nh + X * m_Nw;
  Vector3 nbl = Nc - Y * m_Nh - X * m_Nw;
  Vector3 nbr = Nc - Y * m_Nh + X * m_Nw;

  Vector3 ftl = Fc + Y * m_Fh - X * m_Fw;
  Vector3 ftr = Fc + Y * m_Fh + X * m_Fw;
  Vector3 fbl = Fc - Y * m_Fh - X * m_Fw;
  Vector3 fbr = Fc - Y * m_Fh + X * m_Fw;
  
  m_Planes[PTOP]    = Plane(ntr, ntl, ftl);
  m_Planes[PBOTTOM] = Plane(nbl, nbr, fbr);
  m_Planes[PLEFT]   = Plane(ntl, nbl, fbl);
  m_Planes[PRIGHT]  = Plane(nbr, ntr, fbr);
  m_Planes[PNEAR]   = Plane(ntl, ntr, nbr);
  m_Planes[PFAR]    = Plane(ftr, ftl, fbl);
}
} // Recluse