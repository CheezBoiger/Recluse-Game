// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 NormalColor;
layout (location = 2) out vec4 BrightColor;
layout (location = 3) out vec4 PositionColor;
layout (location = 4) out vec4 RoughMetalColor;

in FRAG_IN {
  vec3 uvw;
} frag_in;


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
  vec2  mousePos;
  ivec2 screenSize;
  vec4  vSun; // Sundir.xyz and w is brightness.
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
} gWorldBuffer;


layout (set = 1, binding = 0) uniform samplerCube skyTexture;

void main()
{
  FragColor = texture(skyTexture, frag_in.uvw);
 
  BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
  NormalColor = vec4(0.0, 0.0, 0.0, 1.0);
  PositionColor = vec4(0.0, 0.0, 0.0, 1.0);
  RoughMetalColor = vec4(0.0, 0.0, 0.0, 1.0);
  // We might just want to work with the final image.

  vec3 glow = (FragColor.rgb - vec3(0.7)) * gWorldBuffer.vSun.w;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  BrightColor = vec4(glow, 1.0);

}