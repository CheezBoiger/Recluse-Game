// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;


out FRAG_IN {
  vec2  uv;
} frag_in;


void main()
{
  frag_in.uv = uv;
  
  gl_Position = vec4(position.xy, 0.0, 1.0);
}