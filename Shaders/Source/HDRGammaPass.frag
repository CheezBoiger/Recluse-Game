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


void main()
{
  // Reference: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/7.bloom/7.bloom_final.fs
  //
  vec3 color = texture(sceneSurface, frag_in.uv).rgb;
  vec3 bloom = texture(bloomSurface, frag_in.uv).rgb;
 
  // Perform an additive blending to the scene surface. This is because
  // we want to be able to enhance bloom areas within the scene texture.
  if (gWorldBuffer.bloomEnabled >= 1) { color += bloom; }
  
  // Extended exposure pass with Reinhard tone mapping. Gamma correction
  // is also enabled.
  vec3 tone = vec3(1.0) - exp(-color * gWorldBuffer.exposure);
  tone = pow(tone, vec3(1.0 / gWorldBuffer.gamma));
  
  vec4 post = vec4(tone, 1.0);
  fragColor = post;
}