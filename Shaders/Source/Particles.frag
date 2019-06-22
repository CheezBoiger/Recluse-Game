// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (location = 0) out vec4 outputColor;
layout (location = 1) out vec4 brightColor;
layout (location = 2) out vec4 rt0;
layout (location = 3) out vec4 rt1;
layout (location = 4) out vec4 rt2;
layout (location = 5) out vec4 rt3;

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

layout (set = 1, binding = 0) uniform ParticleBuffer {
  mat4  model;
  mat4  modelView;
  vec4  hasTexture;
  vec4  globalScale;
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

layout (set = 1, binding = 1) uniform sampler2DArray particleAtlas;

in FragIn {
  vec4  color;
  vec4  clipPos;
  vec2  uv;
  float angle;
  float life;
} frag_in;

vec4 SRGBToLINEAR(vec4 srgbIn)
{
  vec3 linOut = pow(srgbIn.xyz, vec3(2.2));
  return vec4(linOut, srgbIn.w);
}


void main()
{
  vec4 worldPos = frag_in.clipPos / frag_in.clipPos.w;
  worldPos *= gWorldBuffer.global.invViewProj;
  vec3 V = normalize(gWorldBuffer.global.cameraPos.xyz - worldPos.xyz);
  // for test.
  // TODO(): 
  vec4 color = frag_in.color;
  if (particleBuffer.hasTexture.x >= 1.0) {
    vec3 uvw = vec3(frag_in.uv, 0.0);
    uvw.z = mod(
      (particleBuffer.particleMaxAlive - frag_in.life) * particleBuffer.animScale.x, 
      particleBuffer.animScale.y) + particleBuffer.animScale.z;
      
    vec4 t0 = texture(particleAtlas, uvw);
    color.xyz += t0.rgb * particleBuffer.lightFactor.r;
    color.w = t0.a;
    
    if (particleBuffer.fadeIn.x != 0 && frag_in.life >= particleBuffer.fadeIn.x) {
      float m = particleBuffer.particleMaxAlive;
      float ia = mix(0.0, color.w, (m - frag_in.life) / (m - particleBuffer.fadeIn.x));
      color.w = ia;
    }
    
    if (frag_in.life <= particleBuffer.fadeAt) {
      float ia = mix(0.0, color.w, frag_in.life / particleBuffer.fadeAt);
      color.w = ia;
    }
  }
  
  if (color.a < 0.015) {
    discard;
  }
  outputColor = SRGBToLINEAR(color);
  
  vec3 glow = color.rgb - length(V) * 0.2;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  brightColor = vec4(glow, 1.0);
}