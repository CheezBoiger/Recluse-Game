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

  // enable pbr sky, this will render the sky instead of defining one. Uses
  // the primary light information from scene to render out where the sun/moon should
  // go.
  b32            _bRenderPbrSky;

  // If pbr sky rendering is disabled, then user wants to 
  // paste a cubemap sky instead. This must be defined then!
  Texture2D*    _textures[6];


  TextureCube*    GetCubeMap() { return m_cubeTexture; }

  // Get information of the sun light, this will effect how to light works
  // for objects in the scene.
  DirectionalLight* GetSunLight() { return &m_PrimaryLight; }

private:
  DirectionalLight  m_PrimaryLight;
  TextureCube*      m_cubeTexture;
  Vector3           m_betaR;
  Vector3           m_betaM;  
  Vector3           m_sunDir;
  r32               m_earthRadius;
  r32               m_atmosphereRadius;
  r32               m_hR; // Thickness of atmosphere (rayleigh).
  r32               m_hM; // Thickness of atmosphere (mie).
};
} // Recluse