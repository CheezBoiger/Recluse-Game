// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable

#ifndef SHADOW_MAP_OPAQUE

layout (set = 1, binding = 0) uniform MaterialBuffer {
  vec4  color;
  vec4  anisoSpec;
  vec4  offsetUV;
  float opaque;
  float metal;
  float rough;
  float emissive;
  int   hasAlbedo;
  int   hasMetallic;
  int   hasRoughness;
  int   hasNormal;
  int   hasEmissive;
  int   hasAO;
  int   isTransparent;
  int   pad;
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
#ifndef SHADOW_MAP_OPAQUE  
  vec4 alb = matBuffer.color;
  if (matBuffer.hasAlbedo >= 1) {
    vec2 uv0 = fragIn.uv0 + matBuffer.offsetUV.xy;
    alb = texture(albedo, uv0);
  }
  
  if (alb.a < 0.5) {
    discard;
  }
#endif
  //gl_FragDepth = gl_FragCoord.z;  
}