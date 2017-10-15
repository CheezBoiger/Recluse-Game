// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 position;
layout (location = 0) in vec2 uv;

struct FRAG_IN {
  vec4 position;
  vec2 uv;
} frag_in;


layout (binding = 0) uniform UIBuffer {
  mat4 proj;
} ui;


void main()
{
  gl_Position = ui.proj * vec4(position.xy, 0.0, 1.0);
  
  frag_in.position = position;
  frag_in.uv = uv;
}