// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


in FRAG_IN {
  vec3    position;
  float   lodBias;
  vec3    normal;
  int     texIndex;
  vec2    uv0;
  vec2    uv1;
} fragIn;


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

// Texture lookups using the texIndex.
layout (set = 1, binding = 0) uniform sampler2DArray albedo;
layout (set = 1, binding = 1) uniform sampler2DArray normal;
layout (set = 1, binding = 2) uniform sampler2DArray emissive;

layout (location = 0) out vec4 rt0; // albedo
layout (location = 1) out vec4 rt1; // normal
layout (location = 2) out vec4 rt2; // roughmetal.
layout (location = 3) out vec4 rt3; // emission.

void main()
{
}