// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Core/Utility/Vector.hpp"

#define R_RAYTRACING_BIT           (1<<0)
#define R_MESHSHADER_BIT           (1<<1)
#define R_PIXELBARECENTRIC_BIT     (1<<2)

namespace Recluse {


enum FrameBuffering {
  SINGLE_BUFFER = 1,
  DOUBLE_BUFFER = 2,
  TRIPLE_BUFFER = 3
};


enum AntiAliasing {
  AA_None,
  AA_FXAA_2x,
  AA_SMAA_2x
};


enum GraphicsQuality {
  GRAPHICS_QUALITY_NONE = 0,
  GRAPHICS_QUALITY_POTATO = 1,
  GRAPHICS_QUALITY_LOW = 2,
  GRAPHICS_QUALITY_MEDIUM = 3,
  GRAPHICS_QUALITY_HIGH = 4,
  GRAPHICS_QUALITY_ULTRA = 5
};


enum WindowType {
  WindowType_Fullscreen,
  WindowType_Borderless,
  WindowType_Border
};


enum WindowResolution {
  Resolution_800x600,
  Resolution_1200x800,
  Resolution_1280x720,
  Resolution_1440x900,
  Resolution_1920x1080,
  Resolution_1920x1200,
  Resolution_2048x1440, // 2k
  Resolution_3840x2160, // 4k
  Resolution_7680x4320 // 8k
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
  GraphicsQuality  _TextureQuality;

  // Determines the quality and details of shadows in the game world.
  // This will have a performance impact the better the quality. Recommended to
  // Stay at HIGH, since ULTRA is pretty hefty.
  // NOTE(): You must restart the game in order for qualities to take effect!
  //          Otherwise, disabling and enabling shadows can be done in runtime.
  GraphicsQuality    _Shadows;
  
  // Lighting quality affects how well light contributes to the scene. The higher the quality,
  // the better the looks, but will impact performance. 
  GraphicsQuality     _LightQuality;

  // Model quality. 
  GraphicsQuality     _modelQuality;

  // Level of detail, by distance. The higher, the better the models in the world.
  r32             _Lod;

  // render scale determines the viewport surface to render onto.
  r32             _renderScale;

  u32             _shadowMapRes;

  u32             _shadowMapArrayRes;

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

  // setEnable local reflections. Global illumination for local enviroments which adds some 
  // quality to object surfaces. Disabling will improve performance, but reduce quality.
  b32             _EnableLocalReflections;

  // Allow blooms ingame. This allows light to bleed out of emissive, shiny, and metallic objects,
  // depending on how intense the light reflection, or light emission, is.
  b32             _EnableBloom;

  // Allows the the renderer engine to multithread its workload.
  b32             _EnableMultithreadedRendering;

  // Soft shadows. Performs additional computations to produce realistic shadows that better translate 
  // to how large lights would shade an object in a scene by blurring depending on distance from
  // an object who is being casted a shadow, improving its depth perception. Performance may slightly degrade.
  b32             _EnableSoftShadows;

  WindowResolution _Resolution;
  WindowType       _WindowType;
};


const GraphicsConfigParams kDefaultGpuConfigs = {
  DOUBLE_BUFFER,
  AA_None,
  GRAPHICS_QUALITY_ULTRA,
  GRAPHICS_QUALITY_NONE,
  GRAPHICS_QUALITY_HIGH,
  GRAPHICS_QUALITY_HIGH,
  0.0f,
  1.0f,
  2048u,
  512u,
  true,
  true,
  true,
  true,
  true,
  true,
  true,
  Resolution_800x600,
  WindowType_Border
};

} // Recluse