// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


in FRAG_IN {
  vec2 uv;
} frag_in;


// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
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
  int   bloomEnabled;
  int   enableShadows;
  int   enableAA;
} gWorldBuffer;


// camera view and projection.
layout (push_constant) uniform Const {
  mat4 mInvView;
  mat4 mInvProj;
} viewer;


layout (location = 0) out vec4 FragColor;


vec3 GetWorldNormal()
{
  vec2 vViewport = vec2(gWorldBuffer.screenSize);
  vec2 vFragCoord = gl_FragCoord.xy / vViewport;
  vFragCoord = (vFragCoord-0.5)*2.0;
  vec4 vDeviceNormal = vec4(vFragCoord, 0.0, 1.0);
  vec4 vEyeNormal = normalize(viewer.mInvProj * vDeviceNormal);
  vec3 vWorldNormal = normalize(viewer.mInvView * vEyeNormal).xyz;
  return vWorldNormal;
}


float Phase(float alpha, float g)
{
  float a =   3.0 * (1.0 - g*g);
  float b = 2.0 * (2.0 + g*g);
  float c = 1 + alpha*alpha;
  float d = pow(1.0 + g*g - 2.0 * g * alpha, 1.5);
  return (a/b)*(c/d);
}


void main()
{
  float fRayleigh = 1.0;
  vec3  vEyeDir = GetWorldNormal();
  float alpha = dot(vEyeDir, gWorldBuffer.vSun.xyz);
  float fRayleighFactor = Phase(alpha, -0.01) * gWorldBuffer.fRayleigh;
  float fMieFactor = Phase(alpha, gWorldBuffer.fMieDist) * gWorldBuffer.fMie;
  float spot = smoothstep(0.0, 15.0, Phase(alpha, 0.9995)) * gWorldBuffer.vSun.w;
  
  
  
  FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}