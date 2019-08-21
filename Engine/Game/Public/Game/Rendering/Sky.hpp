// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"

#include "Renderer/TextureType.hpp"
#include "Renderer/LightDescriptor.hpp"


namespace Recluse {


// Sky defines the, well, sky... In the form of a cubemap, or skybox.
// This allows the atmosphere to be rendered in physically based format,
// or to instead, allow the dev to use their own images to represent the sky.
class Sky {
public:
  Sky()
    : _skybox(nullptr) { }

  // enable pbr sky, this will render the sky instead of defining one. Uses
  // the primary light information from scene to render out where the sun/moon should
  // go.
  B32            _bRenderPbrSky;

  // If pbr sky rendering is disabled, then user wants to 
  // paste a cubemap sky instead. This must be defined then!
  TextureCube*    _skybox;

  // Get information of the sun light, this will effect how to light works
  // for objects in the scene.
  DirectionalLight* getSunLight() { return &m_PrimaryLight; }

  void              setSkyColor(const Vector3& color);

  void              setMie(const Vector3& bM);

  void              setRayleigh(const Vector3& bR);

  void              setMieDistrib(R32 d);

  void              setScatterStrength(R32 s);

  void              setSkyIntensity(R32 i);

private:
  DirectionalLight  m_PrimaryLight;
  Vector3           m_skyColor;
  Vector3           m_betaR;
  Vector3           m_betaM;  
  Vector3           m_sunDir;
  R32               m_earthRadius;
  R32               m_atmosphereRadius;
  R32               m_hR; // Thickness of atmosphere (rayleigh).
  R32               m_hM; // Thickness of atmosphere (mie).
};
} // Recluse