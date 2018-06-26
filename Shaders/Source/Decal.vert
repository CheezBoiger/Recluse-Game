// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Bounding box and it's uv coordinates. No normal is required as it only defines 
// the bounds of the decal.
layout (location = 0) in vec4 position;
layout (location = 1) in vec2 uv0;
layout (location = 2) in vec2 uv1;

layout (push_constant) uniform DecalTransform {
  mat4      model;
  float     lodBias;
  float     opacity;
} transform;

// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  invView;
  mat4  invProj;
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
  vec4  vSun; // Sundir.xyz and w is brightness.
  vec4  vAirColor;
  float fEngineTime; // total current time of the engine. 
  float fDeltaTime; // elapsed time between frames.
  float gamma;
  float exposure;
  float fRayleigh;
  float fMie;
  float fMieDist;
  float fScatterStrength;
  float fRayleighStength;
  float fMieStength;
  float fIntensity;
  int   bloomEnabled;
  int   enableShadows;
  int   enableAA;
  ivec2 pad;
} gWorldBuffer;

out FRAG_IN {
  vec3  positionCS;   // position of boundingbox in clip space.
  float lodBias;      // level of detail.
  vec2  uv0;          // uv coords.
  vec2  uv1;          // second uv coords.
  vec4  opacity;      // opacity.
} fragIn;

void main()
{
  fragIn.lodBias = transform.lodBias;
  fragIn.opacity.x = transform.opacity;
  fragIn.uv0 = uv0;
  fragIn.uv1 = uv1;
  
  vec4 clipPos = gWorldBuffer.viewProj * transform.model * position;
  gl_Position = clipPos;
  fragIn.positionCS = clipPos.xyz;
}