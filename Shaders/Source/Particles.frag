// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 outputColor;
layout (location = 1) out vec4 brightColor;
layout (location = 2) out vec4 rt0;
layout (location = 3) out vec4 rt1;
layout (location = 4) out vec4 rt2;
layout (location = 5) out vec4 rt3;

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

layout (set = 1, binding = 0) uniform ParticleBuffer {
  float level[16];
  mat4  model;
  mat4  modelView;
  float fadeAt;
  float fadeThreshold;
  float angleThreshold;
  float rate;
  float lifeTimeScale;
  float particleMaxAlive;
  float maxParticles;
  float isWorldSpace;
} particleBuffer;

layout (set = 1, binding = 1) uniform sampler2DArray particleAtlas;

in FragIn {
  vec4  color;
  vec4  worldPos;
  vec2  uv;
  float angle;
  float life;
} frag_in;


void main()
{
  vec3 V = normalize(gWorldBuffer.cameraPos.xyz - frag_in.worldPos.xyz);
  // for test.
  // TODO(): 
  vec4 color = frag_in.color;
  
  if (color.a < 0.5) {
    discard;
  }
  outputColor = color;
  
  vec3 glow = color.rgb - length(V) * 0.2;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  brightColor = vec4(glow, 1.0);
}