// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   texcoord0;
layout (location = 3) in vec2   texcoord1;


#define MAX_BONES     64


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
  ivec2 screenSize;
  ivec2 pad;
} gWorldBuffer;


layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  vec4  color;
  float levelOfDetail;
  float transparency;
  int   hasAlbedo;
  int   hasMetallic;
  int   hasRoughness;
  int   hasNormal;
  int   hasEmissive;
  int   hasAO;
  int   hasBones; 
  int   isTransparent;
  ivec2 pad;
} objBuffer;


out FRAG_IN {
  vec3  position;
  float lodBias;
  vec3  normal;
  float pad1;
  vec2  texcoord0;
  vec2  texcoord1;
} frag_in;


void main()
{
  vec4 worldPosition = position;
 
  worldPosition = objBuffer.model * worldPosition;
  
  frag_in.position = worldPosition.xyz;
  frag_in.texcoord0 = texcoord0;
  frag_in.texcoord1 = texcoord1;
  frag_in.normal = normalize(objBuffer.normalMatrix * normal).xyz;
  
  gl_Position = gWorldBuffer.viewProj * worldPosition;
}