// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 tex0;
layout (location = 3) in vec2 tex1;


// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  viewProj;
  vec4  cameraPos;
  vec4  l_plane;
  vec4  r_plane;
  vec4  t_plane;
  vec4  b_plane;
  vec4  n_plane;
  vec4  f_plane;
  vec2  mousePos;
  ivec2 screenSize;
  float gamma;
  float exposure;
  int   bloomEnabled;
  int   enableShadows;
  int   enableAA;
} gWorldBuffer;


// Skydome usually follows the camera.
layout (set = 1, binding 0) uniform SkyDomeBuffer {
  mat4 model;
} skyDomeBuffer;


out FRAG_IN {
  vec3  position;
  float pad0;
  vec3  normal;
  float pad1;
  vec2  tex0;
  vec2  tex1;
} frag_in;


void main()
{
  frag_in.position = position.xyz;
  frag_in.normal = normal.xyz;
  frag_in.tex0 = tex0;
  frag_in.tex1 = tex1;
  
  gl_Position = gWorldBuffer.viewProj * skyDomeBuffer.model * vec4(position.xyz, 1.0);
}