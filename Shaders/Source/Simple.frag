// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"
layout (location = 0) out vec4 vFragColor;
layout (location = 1) out vec4 vBrightColor;
layout (location = 2) out vec4 rt0;
layout (location = 3) out vec4 rt1;
layout (location = 4) out vec4 rt2;
layout (location = 5) out vec4 rt3;

layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  float lod;          // Level of Detail
  int   hasBones; 
  float w0;
  float w1;
} objBuffer;

in FRAG_IN {
  vec3 position;
  float lodBias;
  vec3 normal;
  float pad1;
  vec2 texcoord0;
  vec2 texcoord1;
} frag_in;


layout (push_constant) uniform PushConst {
  vec4 color;
} pushInfo;


void main()
{
  vec4 color = pushInfo.color;
  vFragColor = color;
}