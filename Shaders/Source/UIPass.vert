// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable


layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;


out FRAG_IN {
  vec2 position;
  vec2 uv;
} frag_in;


layout (set = 0, binding = 0) uniform UIBuffer {
  mat4 model;
} ui_buffer;


void main()
{
  gl_Position = vec4(position, 0.0, 1.0);
  frag_in.uv = uv;
  frag_in.position = position;
}