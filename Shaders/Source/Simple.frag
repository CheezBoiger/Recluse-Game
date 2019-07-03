// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (location = 0) out vec4 vFragColor;

in FRAG_IN {
  vec3 position;
  float lodBias;
  vec3 normal;
  float pad1;
  vec2 texcoord0;
  vec2 texcoord1;
} frag_in;

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

layout (push_constant) uniform PushConst {
  layout (offset = 80) vec4 color;
} pushInfo;


void main()
{
  vec4 color = pushInfo.color;
  vFragColor = color;
}