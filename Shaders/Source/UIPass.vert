// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable


layout (location = 0) in vec4 position;
layout (location = 1) in vec2 uv;


out FRAG_IN {
  vec4 position;
  vec2 uv;
  float pad[2];
} frag_in;


// May want to add more values to create some interesting effects in the UI.
layout (set = 0, binding = 0) uniform UIBuffer {
  mat4 proj;
} ui;

void main()
{
  gl_Position = ui.proj * vec4(position.xy, 0.0, 1.0);
  frag_in.uv = uv;
  frag_in.position = position;
}