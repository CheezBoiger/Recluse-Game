// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable


layout (location = 0) in vec2 v2Position;
layout (location = 1) in vec2 v2Uv;
layout (location = 2) in vec4 v4Color;


out FRAG_IN {
  vec2 v2Position;
  vec2 v2Uv;
  vec4 v4Color;
} frag_in;


void main()
{
  gl_Position = vec4(v2Position, 0.0, 1.0);
  frag_in.v2Uv = v2Uv;
  frag_in.v2Position = v2Position;
  frag_in.v4Color = v4Color;
}