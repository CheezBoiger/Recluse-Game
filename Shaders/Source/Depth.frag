// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

#ifndef DEPTH_OPAQUE

layout (set = 1, binding = 0) uniform MaterialBuffer {
  Material mat;
} matBuffer;

layout (set = 1, binding = 1) uniform sampler2D albedo;
layout (set = 1, binding = 2) uniform sampler2D roughnessMetallic;
layout (set = 1, binding = 3) uniform sampler2D normal;
layout (set = 1, binding = 4) uniform sampler2D ao;
layout (set = 1, binding = 5) uniform sampler2D emissive;

in FragIn {
  vec2 uv0;
  vec2 uv1;
} fragIn;

#endif

void main()
{
  // TODO(): We may also want to add in transparency map as well?
#ifndef DEPTH_OPAQUE  
  vec4 alb = matBuffer.mat.color;
  if (matBuffer.mat.hasAlbedo >= 1) {
    vec2 uv0 = fragIn.uv0 + matBuffer.mat.offsetUV.xy;
    alb = texture(albedo, uv0);
  }
  
  if (alb.a < 0.5) {
    discard;
  }
#endif
  //gl_FragDepth = gl_FragCoord.z;  
}