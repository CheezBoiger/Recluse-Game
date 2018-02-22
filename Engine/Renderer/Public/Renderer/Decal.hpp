// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "RHI/VulkanConfigs.hpp"


namespace Recluse {


class Texture2D;


// Maximum decals that can be tagged into the world.
#define DECAL_MAX         4

// Decal structure.
struct Decal {
  u32           _id;
  Texture2D*    _texture;
};

// Decal manager engine. this engine keeps track, and updates,
// decals within the game world.
class DecalEngine {
public:
  DecalEngine() { }

  // Uhhh, probably don't want to do this...
  Decal&      GetDecal(size_t idx) { return m_decals[idx]; }

private:
  Decal       m_decals[DECAL_MAX];
};
} // Recluse