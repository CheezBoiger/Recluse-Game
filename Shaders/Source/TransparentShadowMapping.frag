// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable


layout (location = 0) out vec4 outColor;


layout (set = 0, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  float lod;          // Level of Detail
  int   hasBones; 
} objBuffer;


layout (set = 1, binding = 0) uniform MaterialBuffer {
  vec4  color;
  vec4  anisoSpec;
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


out FragIn {
  vec2 uv0;
  vec2 uv1;
} fragIn;


void main()
{
  vec4 alb = texture(albedo, uv0);
  outColor = vec4(alb.rgb, 1.0);
}