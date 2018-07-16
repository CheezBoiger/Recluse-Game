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
  AA_SMAA_2x
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


class GraphicsConfigParams {
public:
  // Determine draw buffering, and number of back buffers the renderer will use for 
  // presenting onto the display.
  // This allows for smoother quality of frame transitions and input.
  FrameBuffering  _Buffering;

  // Determine the antialiasing quality of the renderer. This helps reduce jagged lines,
  // or "jaggies", by smoothing them out. Most anti aliasing techniques known are software
  // methods, while hardware methods, like MSAA, are also common. Recommended to turn this 
  // off if the display resolution is above 1920x1080 (as it doesn't really need this.)
  AntiAliasing    _AA;

  // Quality of textures ingame. The higher the quality, the more work the renderer needs to sample
  // from texture images.
  TextureQuality  _TextureQuality;

  // Determines the quality and details of shadows in the game world.
  // This will have a performance impact the better the quality. Recommended to
  // Stay at HIGH, since ULTRA is pretty hefty.
  // NOTE(): You must restart the game in order for qualities to take effect!
  //          Otherwise, disabling and enabling shadows can be done in runtime.
  ShadowDetail    _Shadows;

  // Level of detail, by distance. The higher, the better the models in the world.
  r32             _Lod;

  // Allow vertical sync to reduce tearing of frames. This is useful if the display can only 
  // refresh at max 60 Hz. If this monitor can achieve higher refresh rates, this feature may only
  // have negative impacts, and is best to leave off. This may have a slight impact
  // in input response.
  b32             _EnableVsync;

  // Enables chromatic aberration. This allows chromatic distortion
  // by which color fringing occurs and the light fails to focus all colors in one convergence point, 
  // resulting in rgb colors offsetting.
  // This also applies to water and certain translucent objects.
  b32             _EnableChromaticAberration;

  // Post processing affects. This includes HDR.
  b32             _EnablePostProcessing;

  // Enable local reflections. Global illumination for local enviroments which adds some 
  // quality to object surfaces. Disabling will improve performance, but reduce quality.
  b32             _EnableLocalReflections;

  // Allow blooms ingame. This allows light to bleed out of emissive, shiny, and metallic objects,
  // depending on how intense the light reflection, or light emission, is.
  b32             _EnableBloom;

  // Allows the the renderer engine to multithread its workload.
  b32             _EnableMultithreadedRendering;
};


const GraphicsConfigParams kDefaultGpuConfigs = {
  DOUBLE_BUFFER,
  AA_None,
  TEXTURE_QUALITY_ULTRA,
  SHADOWS_NONE,
  1.0f,
  true,
  true,
  true,
  true,
  true,
  true
};

} // Recluse