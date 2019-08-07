// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

in FRAG_IN {
  vec4    posCS;
  vec4    opacity;
  vec4   lodBias;
} fragIn;

// Texture lookups using the texIndex.
layout (set = 1, binding = 0) uniform sampler2D albedo;
layout (set = 1, binding = 1) uniform sampler2D normal;
layout (set = 1, binding = 2) uniform sampler2D emissive;

// Depth from gbuffer pass. Readonly.
layout (set = 2, binding = 3) uniform sampler2D gDepth;

layout (location = 0) out vec4 rt0; // albedo
layout (location = 1) out vec4 rt1; // normal
layout (location = 2) out vec4 rt2; // roughmetal.
layout (location = 3) out vec4 rt3; // emissive.

void main()
{
  // coordinates of screen from gl_FragCoord, which is in window space.
  vec2 screenPos = fragIn.posCS.xy / fragIn.posCS.w;
  vec2 depthUV = screenPos * vec2(0.5, 0.5) + 0.5;
  float depth = texture( gDepth, depthUV ).r;
  
  vec4 tempPosWS = vec4(depthUV * 2.0 - 1.0, depth, 1.0);
  vec4 posWS = gWorldBuffer.global.invView * tempPosWS;
  posWS = posWS / posWS.w;
}



