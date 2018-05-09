// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Renderer/TextureType.hpp"


namespace Recluse {


class Skybox {
public:

  // enable pbr sky, this will render the sky instead of defining one. Uses
  // the primary light information from scene to render out where the sun/moon should
  // go.
  b32            _bRenderPbrSky;

  // If pbr sky rendering is disabled, then user wants to 
  // paste a cubemap sky instead. This must be defined then!
  Texture2D*    _textures[6];


  TextureCube*    GetCubeMap() { return m_cubeTexture; }

private:
  TextureCube*   m_cubeTexture;
};
} // Recluse