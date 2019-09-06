// Copyright (c) 2018 Recluse Project. All rights reserved.
#ifndef GLOBALS_H
#define GLOBALS_H

// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.

struct GlobalBuffer {
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
  vec4  fov;
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
};


struct Material {
  vec4  color;
  vec4  anisoSpec;
  vec4  offsetUV;
  float opaque;
  float metal;
  float rough;
  float emissive;
  int   hasAlbedo;
  int   hasMetallic;
  int   hasRoughness;
  int   hasNormal;
  int   hasEmissive;
  int   hasAO;
  int   isTransparent;
  int   pad;
};


struct Model {
  mat4  model;
  mat4  normalMatrix;
  float lod;          // Level of Detail
  int   hasJoints; 
  float w0;
  float w1;
};


vec3 getPosition(vec2 uv, float depth, in mat4 invViewProj)
{

  vec4 clipPos = vec4(uv.xy, depth, 1.0);
  vec4 worldPos = invViewProj * clipPos;
  worldPos /= worldPos.w;
  return worldPos.xyz;
}
#endif // GLOBALS_H