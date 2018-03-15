// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 fragColor;

in FRAG_IN {
  vec2 position;
  vec2 uv;
} frag_in;


layout (set = 0, binding = 0) uniform sampler2D sceneSurface;
layout (set = 0, binding = 1) uniform sampler2D bloomSurface;


layout (set = 0, binding = 2) uniform GlobalBuffer {
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


vec3 Uncharted2Tonemap(vec3 x)
{
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}


void main()
{
  // Reference: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/7.bloom/7.bloom_final.fs
  //
  vec3 color = texture(sceneSurface, frag_in.uv).rgb;
  vec3 bloom = texture(bloomSurface, frag_in.uv).rgb;
 
  // Perform an additive blending to the scene surface. This is because
  // we want to be able to enhance bloom areas within the scene texture.
  if (gWorldBuffer.bloomEnabled >= 1) { color += bloom; }
  
  // Extended exposure pass with Uncharted 2 tone mapping. Gamma correction
  // is also enabled.
  vec3 tone = Uncharted2Tonemap(color * gWorldBuffer.exposure);
  tone = tone * (1.0 / Uncharted2Tonemap(vec3(11.2)));
  
  // Gamma correction.
  tone = pow(tone, vec3(1.0 / gWorldBuffer.gamma));
  
  vec4 post = vec4(tone, 1.0);
  fragColor = post;
}