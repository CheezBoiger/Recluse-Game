// Copyright (c) 2019 Recluse Project. All rights reserved.
#include "Rendering/Sky.hpp"

#include "Renderer/Renderer.hpp"


namespace Recluse {


void Sky::SetSkyColor(const Vector3& color)
{
  gRenderer().GlobalData()->_vAirColor = color; gRenderer().UpdateSky();
}


void Sky::SetMie(const Vector3& bM)
{
  gRenderer().GlobalData()->_Mie = bM.x; gRenderer().UpdateSky();
}


void Sky::SetRayleigh(const Vector3& bR)
{
  gRenderer().GlobalData()->_Rayleigh = bR.x; gRenderer().UpdateSky();
}


void Sky::SetMieDistrib(r32 d)
{
  gRenderer().GlobalData()->_MieDist = d; gRenderer().UpdateSky();
}


void Sky::SetScatterStrength(r32 s)
{
  gRenderer().GlobalData()->_fScatterStrength = s; gRenderer().UpdateSky();
}


void Sky::SetSkyIntensity(r32 i)
{
  gRenderer().GlobalData()->_fIntensity = i; gRenderer().UpdateSky();
}
} // Recluse
