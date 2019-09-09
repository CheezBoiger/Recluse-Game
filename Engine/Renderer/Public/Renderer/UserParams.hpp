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


enum RenderResolution {
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
  // Number of desired frame buffer images to use. This is the number of images 
  // available for your GPU to render with, as they will help unblock gpu rendering while
  // the OS is busy trying to present the rendered frame. Your GPU can then use the finished
  // resources from the currently displaying image to render. Complicated, I know... This is 
  // hardware dependent, so you can not always gaurantee your desired image count will be exact...
  U32 _desiredSwapImages;
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
  R32             _Lod;

  // render scale determines the viewport surface to render onto.
  R32             _renderScale;

  // Determines the resolution quality of sunlight shadow, which comprises of multiple maps rendering
  // objects in the scene, for close to far distances from the viewer. Increasing this will improve
  // quality of shadows, but have a small impact to performance.
  U32             _cascadeShadowMapRes;

  // Number of sunlight shadow maps to use corresponding near and far distances from the viewer.
  // You generally only need 4 maps, but can reduce this for performance, at the cost of quality for 
  // far away objects (objects far away will not have shadows.)
  U32             _numberCascadeShadowMaps;

  // Resolution of shadows for point lights (lamps, torches, spotlights etc...) Increasing this will 
  // give out better quality shadows for these lights, at the cost of performance.
  U32             _shadowMapArrayRes;

  // Allow vertical sync to reduce tearing of frames. This is useful if the display can only 
  // refresh at max 60 Hz. If this monitor can achieve higher refresh rates, this feature may only
  // have negative impacts, and is best to leave off. This may have a slight impact
  // in input response.
  B32             _EnableVsync;

  // Enables chromatic aberration. This allows chromatic distortion
  // by which color fringing occurs and the light fails to focus all colors in one convergence point, 
  // resulting in rgb colors offsetting.
  // This also applies to water and certain translucent objects.
  B32             _EnableChromaticAberration;

  // Post processing affects. This includes HDR.
  B32             _EnablePostProcessing;

  // setEnable local reflections. Global illumination for local enviroments which adds some 
  // quality to object surfaces. Disabling will improve performance, but reduce quality.
  B32             _EnableLocalReflections;

  // Allow blooms ingame. This allows light to bleed out of emissive, shiny, and metallic objects,
  // depending on how intense the light reflection, or light emission, is.
  B32             _EnableBloom;

  // Allows the the renderer engine to multithread its workload.
  B32             _EnableMultithreadedRendering;

  // Soft shadows. Performs additional computations to produce realistic shadows that better translate 
  // to how large lights would shade an object in a scene by blurring depending on distance from
  // an object who is being casted a shadow, improving its depth perception. Performance may slightly degrade.
  B32             _enableSoftShadows;

  // Option to enable a limit to the frame rate of your GPU (if rendering to quickly, engine will attempt to 
  // slow down the rate.)
  B32             _enableFrameLimit;

  // Frame rate limit, should the GPU be rendering too quickly, engine will attempt to slow it down to the 
  // desired frame limit range.
  U32             _frameLimit;

  // Resolution of the scene rendered in the GPU. The better the quality of the the rendering resolution, the 
  // overall better the image will look, at the cost of performance, since the GPU will have to work harder
  // to produce the frame. This is difference from your window resolution, as the OS adapter is meant to map the
  // GPU frame image to the window of your screen.
  RenderResolution _Resolution;

  // Type of window that your GPU is rendering for.
  WindowType       _WindowType;
};


const GraphicsConfigParams kDefaultGpuConfigs = {
  3,
  DOUBLE_BUFFER,
  AA_None,
  GRAPHICS_QUALITY_ULTRA,
  GRAPHICS_QUALITY_NONE,
  GRAPHICS_QUALITY_HIGH,
  GRAPHICS_QUALITY_HIGH,
  0.0f,
  1.0f,
  2048u,
  4,
  512u,
  true,
  true,
  true,
  true,
  true,
  true,
  true,
  false,
  120,
  Resolution_800x600,
  WindowType_Border
};

} // Recluse