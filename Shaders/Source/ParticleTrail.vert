// Copyright (c) 2018 Recluse Project. All rights reserved.

#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (location = 0) in vec4 currPosition;
layout (location = 1) in vec4 prevPosition;
layout (location = 2) in vec4 nextPosition;
layout (location = 3) in vec4 color;
layout (location = 4) in float radius;


out VertOut {
  vec4 currWPos;
  vec4 prevWPos;
  vec4 nextWPos;
  vec4 color;
  float radius;
} vert_out; 


void main()
{
  vec4 currWorldPos = vec4(currPosition.xyz, 1.0);
  vec4 prevWorldPos = vec4(prevPosition.xyz, 1.0);
  vec4 nextWorldPos = vec4(nextPosition.xyz, 1.0);
  
  gl_Position = currWorldPos;
  vert_out.currWPos = currWorldPos;
  vert_out.prevWPos = prevWorldPos;
  vert_out.nextWPos = nextWorldPos;
  vert_out.color = color;
  vert_out.radius = radius;
}