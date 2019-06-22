// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (location = 0) out vec4 outputColor;
layout (location = 1) out vec4 brightColor;
layout (location = 2) out vec4 rt0;
layout (location = 3) out vec4 rt1;
layout (location = 4) out vec4 rt2;
layout (location = 5) out vec4 rt3;

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

in FragIn {
  vec4 color;
} frag_in; 


void main()
{
  outputColor = vec4(frag_in.color);
}