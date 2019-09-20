// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   offsetPos;
layout (location = 2) in vec4   velocity;
layout (location = 3) in vec4   initVelocity;
layout (location = 4) in vec4   acceleration;
layout (location = 5) in vec4   color;
layout (location = 6) in vec4   particleInfo; // x = angle, y = size, z = weight, w = life.
layout (location = 7) in vec4   camDist;

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

layout (set = 1, binding = 0) uniform ParticleBuffer {
  mat4  model;
  mat4  modelView;
  vec4  hasTexture;
  vec4  lightFactor;
  vec4  angleRate;
  vec4  fadeIn;
  vec4  animScale;
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
  float gl_PointSize;
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
  gl_Position = gWorldBuffer.global.view * worldPos;
  gl_PointSize = 1.0;
}