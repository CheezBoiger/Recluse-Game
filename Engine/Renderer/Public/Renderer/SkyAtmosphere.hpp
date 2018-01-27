// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Renderer/Renderer.hpp"

#include <array>

namespace Recluse {


class Texture;
class Semaphore;


class Sky {
public:
  Sky();

  void                    Initialize();
  void                    CleanUp();


  // RenderTextures that will be used as attachments,
  // and will be loaded onto a cubemap for the skybox.
  std::array<Texture*, 6> m_RenderTextures;
  Texture*                m_CubeMap;
  Semaphore*              m_AtmosphereSema;
};
} // Recluse 