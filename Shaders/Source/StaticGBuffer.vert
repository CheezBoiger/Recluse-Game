// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   texcoord0;
layout (location = 3) in vec2   texcoord1;


#if defined(INCLUDE_MORPH_TARGET_ANIMATION)
// For morph target animation, we must define our position layout, above, as the 
// default pose (or the bind pose.) Source pose is defined by this layout.
layout (location = 4) in vec4   position0;
layout (location = 5) in vec4   normal0;
layout (location = 6) in vec2   uv00;
layout (location = 7) in vec2   uv10;

// Destination pose is defined here.
layout (location = 8) in vec4  position1;
layout (location = 9) in vec4  normal1;
layout (location = 10) in vec2  uv01;
layout (location = 11) in vec2  uv11;
#endif


#define MAX_BONES     64


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
  int   bloomEnabled;
  int   enableShadows;
  int   enableAA;
  ivec2 pad;
} gWorldBuffer;


layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  float lod;
  int   hasBones;
  float w0;
  float w1;
} objBuffer;


out FRAG_IN {
  vec3  position;
  float lodBias;
  vec3  normal;
  float pad1;
  vec2  texcoord0;
  vec2  texcoord1;
} frag_in;


#if defined(RENDER_ENV_MAP)
layout (push_constant) uniform Camera {
  mat4 viewProj;
} viewer;
#endif

void main()
{
  vec4 worldPosition = position;
  vec4 worldNormal = normal;
  vec2 temp_uv0 = texcoord0;
  vec2 temp_uv1 = texcoord1;
  
#if defined(INCLUDE_MORPH_TARGET_ANIMATION)
  float w0 = objBuffer.w0;
  float w1 = objBuffer.w1;
  vec4  morphPositionDiff0 = position0;
  vec4  morphPositionDiff1 = position1;
  vec4  morphNormalDiff0 = normal0;
  vec4  morphNormalDiff1 = normal1;
  vec2  morphUV00 = uv00;
  vec2  morphUV01 = uv01;
  vec2  morphUV10 = uv10;
  vec2  morphUV11 = uv11;
  worldPosition += morphPositionDiff0 * w0;
  worldPosition += morphPositionDiff1 * w1;
  worldNormal += morphNormalDiff0 * w0;
  worldNormal += morphNormalDiff1 * w1;
  temp_uv0 += morphUV00 * w0;
  temp_uv0 += morphUV01 * w1;
  temp_uv1 += morphUV10 * w0;
  temp_uv1 += morphUV11 * w1;
#endif
 
  worldPosition = objBuffer.model * worldPosition;
  
#if defined(ENABLE_WATER_RENDERING)
  gl_ClipDistance[0] = dot(worldPosition, gWorldBuffer.clipPlane0);  
#endif

  frag_in.position = worldPosition.xyz;
  frag_in.texcoord0 = temp_uv0;
  frag_in.texcoord1 = temp_uv1;
  frag_in.normal = normalize(objBuffer.normalMatrix * worldNormal).xyz;
#if !defined(RENDER_ENV_MAP)
  gl_Position = gWorldBuffer.viewProj * worldPosition;
#else
  gl_Position = viewer.viewProj * worldPosition;
#endif
}