// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


in FRAG_IN {
  vec3  position;
  float pad0;
  vec3  normal;
  float pad1;
  vec2  tex0;
  vec2  tex1;
} frag_in;


layout (set = 1, binding = 1) uniform sampler2D texture0;
layout (set = 1, binding = 2) uniform sampler2D texture1;


layout (location = 0) out vec4 FragColor;




void main()
{
  FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}