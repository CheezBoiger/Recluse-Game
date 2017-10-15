// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 0) in vec2 uv;

struct FRAG_IN {
  vec2 position;
  vec2 uv;
} frag_in;


void main()
{
  gl_Position = vec4(position, 0.0, 1.0);
  
  frag_in.position = position;
  frag_in.uv = uv;
}