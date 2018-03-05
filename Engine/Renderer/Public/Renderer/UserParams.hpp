// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Core/Utility/Vector.hpp"

namespace Recluse {


enum FrameBuffering {
  SINGLE_BUFFER,
  DOUBLE_BUFFER,
  TRIPLE_BUFFER
};


enum WindowType {
  WINDOW_BORDER,
  WINDOW_BORDERLESS,
  WINDOW_FULLSCREEN
};


enum AntiAliasing {
  AA_None,
  AA_FXAA_2x,
  AA_FXAA_4x,
  AA_FXAA_8x
};


enum ShadowDetail {
  SHADOWS_NONE = 0,
  SHADOWS_POTATO = 1,
  SHADOWS_LOW = 2,
  SHADOWS_MEDIUM = 3,
  SHADOWS_HIGH = 4,
  SHADOWS_ULTRA = 5
};


// TODO(): No effect yet, until we at least make our textures in the future.
enum TextureQuality {
  TEXTURE_QUALITY_LOW,
  TEXTURE_QUALITY_MEDIUM,
  TEXTURE_QUALITY_HIGH,
  TEXTURE_QUALITY_ULTRA
};


class GpuConfigParams {
public:
  // Determine draw buffering, and number of back buffers the renderer will use.
  // This allows for smoother quality of frame transitions (theoretically).
  FrameBuffering  _Buffering;
  // Determine the antialiasing quality of the renderer.
  AntiAliasing    _AA;

  // Quality of textures ingame.
  TextureQuality  _TextureQuality;

  // Determines the quality and details of shadows in the game world.
  // This will have a performance impact the better the quality. 
  // NOTE(): You must restart the game in order for qualities to take effect!
  //          Otherwise, disabling and enabling shadows can be done in runtime.
  ShadowDetail    _Shadows;
  // Level of detail, by distance.
  r32             _Lod;
  // Allow vertical sync to reduce tearing of frames. This may have a slight impact
  // in input response.
  u32             _EnableVsync;
};


const GpuConfigParams kDefaultGpuConfigs = {
  DOUBLE_BUFFER,
  AA_None,
  TEXTURE_QUALITY_ULTRA,
  SHADOWS_NONE,
  1.0f,
  true
};

} // Recluse