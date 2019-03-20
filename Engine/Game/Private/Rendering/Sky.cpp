// Copyright (c) 2019 Recluse Project. All rights reserved.
#include "Rendering/Sky.hpp"

#include "Renderer/Renderer.hpp"


namespace Recluse {


void Sky::setSkyColor(const Vector3& color)
{
  gRenderer().getGlobalData()->_vAirColor = color; gRenderer().updateSky();
}


void Sky::setMie(const Vector3& bM)
{
  gRenderer().getGlobalData()->_Mie = bM.x; gRenderer().updateSky();
}


void Sky::setRayleigh(const Vector3& bR)
{
  gRenderer().getGlobalData()->_Rayleigh = bR.x; gRenderer().updateSky();
}


void Sky::setMieDistrib(r32 d)
{
  gRenderer().getGlobalData()->_MieDist = d; gRenderer().updateSky();
}


void Sky::setScatterStrength(r32 s)
{
  gRenderer().getGlobalData()->_fScatterStrength = s; gRenderer().updateSky();
}


void Sky::setSkyIntensity(r32 i)
{
  gRenderer().getGlobalData()->_fIntensity = i; gRenderer().updateSky();
}
} // Recluse
