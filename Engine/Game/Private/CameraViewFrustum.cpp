// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "CameraViewFrustum.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
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
  m_pCamera = camera;
}


void CCamViewFrustum::ConfigureFrustum()
{
  // Might need to figure out this Tang thing...
  r32 Tang = (r32)tanf(Radians(m_pCamera->FoV() * 0.5f));
  m_Nh = m_pCamera->Near() * Tang;
  m_Nw = m_Nh * m_pCamera->Aspect();
  m_Fh = m_pCamera->Far() * Tang;
  m_Fw = m_Fh * m_pCamera->Aspect();
}


void CCamViewFrustum::Update()
{
  if (!m_pCamera) {
    R_DEBUG(rError, "No camera set to calc frustums from!\n");
    return;
  }

  if (!m_pCamera->Culling()) return;
  ConfigureFrustum();
  
  Vector3 l = m_pCamera->LookDir();
  Vector3 p = m_pCamera->Position();
  Vector3 u = m_pCamera->Up();

  Vector3 Nc, Fc, X, Y, Z;

  Z = m_pCamera->Front().Normalize();
  X = -(u ^ Z).Normalize();
  Y = Z ^ X;

  Nc = p - Z * m_pCamera->Near();
  Fc = p - Z * m_pCamera->Far();

  Vector3 ntl = Nc + Y * m_Nh - X * m_Nw;
  Vector3 ntr = Nc + Y * m_Nh + X * m_Nw;
  Vector3 nbl = Nc - Y * m_Nh - X * m_Nw;
  Vector3 nbr = Nc - Y * m_Nh + X * m_Nw;

  Vector3 ftl = Fc + Y * m_Fh - X * m_Fw;
  Vector3 ftr = Fc + Y * m_Fh + X * m_Fw;
  Vector3 fbl = Fc - Y * m_Fh - X * m_Fw;
  Vector3 fbr = Fc - Y * m_Fh + X * m_Fw;
  
  _Planes[PTOP]    = Plane(ntr, ntl, ftl);
  _Planes[PBOTTOM] = Plane(nbl, nbr, fbr);
  _Planes[PLEFT]   = Plane(ntl, nbl, fbl);
  _Planes[PRIGHT]  = Plane(nbr, ntr, fbr);
  _Planes[PNEAR]   = Plane(ntl, ntr, nbr);
  _Planes[PFAR]    = Plane(ftr, ftl, fbl);
}
} // Recluse