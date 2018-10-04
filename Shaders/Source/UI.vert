// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable


layout (location = 0) in vec2 v2Position;
layout (location = 1) in vec2 v2Uv;
layout (location = 2) in vec4 v4Color;


out FRAG_IN {
  vec2 v2Position;
  vec2 v2Uv;
  vec4 v4Color;
} frag_in;


// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  invView;
  mat4  invProj;
  mat4  viewProj;
  mat4  invViewProj;
  vec4  cameraPos;
  vec4  l_plane;
  vec4  r_plane;
  vec4  t_plane;
  vec4  b_plane;
  vec4  n_plane;
  vec4  f_plane;
  vec4  clipPlane0;
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
  float zNear;
  float zFar;
  int   bloomEnabled;
  int   enableShadows;
  int   enableAA;
} gWorldBuffer;


void main()
{
  vec2 localPos = v2Position / vec2(gWorldBuffer.screenSize);
  localPos = 2.0*localPos - 1.0;
  gl_Position = vec4(localPos, 0.0, 1.0);
  frag_in.v2Uv = v2Uv;
  frag_in.v2Position = localPos;
  frag_in.v4Color = v4Color;
}