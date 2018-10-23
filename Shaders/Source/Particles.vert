// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   offsetPos;
layout (location = 2) in vec4   velocity;
layout (location = 3) in vec4   initVelocity;
layout (location = 4) in vec4   acceleration;
layout (location = 5) in vec4   color;
layout (location = 6) in vec4   particleInfo; // x = angle, y = size, z = weight, w = life.
layout (location = 7) in vec4   camDist;


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
  vec4  hasTexture;
  vec4  lightFactor;
  vec4  angleRate;
  vec4  fadeIn;
  float fadeAt;
  float fadeThreshold;
  float angleThreshold;
  float rate;
  float lifeTimeScale;
  float particleMaxAlive;
  float maxParticles;
  float isWorldSpace;
} particleBuffer;

out VertOut {
  vec4 color;
  vec4 worldPos;
  float angle;
  float size;
  float weight;
  float life;
} vert_out;

out gl_PerVertex {
  vec4 gl_Position;
};

void main()
{
  vec4 worldPos = vec4(position.xyz, 1.0);
  
  if (particleBuffer.isWorldSpace <= 0.0) {
    worldPos = particleBuffer.model * worldPos;
  }
  vert_out.worldPos = worldPos;
  vert_out.color = color;
  vert_out.angle = particleInfo.x;
  vert_out.size = particleInfo.y;
  vert_out.weight = particleInfo.z;
  vert_out.life = particleInfo.w;
  gl_Position = gWorldBuffer.view * worldPos;
}