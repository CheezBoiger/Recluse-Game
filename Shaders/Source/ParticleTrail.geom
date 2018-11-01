// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;


in VertOut {
  vec4 currWPos;
  vec4 prevWPos;
  vec4 nextWPos;
} vert_out[];


out FragIn {
} frag_in;


void main()
{
  

  EndPrimitive();
}