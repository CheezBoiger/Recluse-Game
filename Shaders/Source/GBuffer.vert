// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   uv0;
layout (location = 3) in vec2   uv1;
layout (location = 4) in vec4   jointWeights;
layout (location = 5) in ivec4  jointIDs;

#if defined(INCLUDE_MORPH_TARGET_ANIMATION)
// For morph target animation, we must define our position layout, above, as the 
// default pose (or the bind pose.) Source pose is defined by this layout.
layout (location = 6) in vec4   position0;
layout (location = 7) in vec4   normal0;
layout (location = 8) in vec2   uv00;
layout (location = 9) in vec2   uv10;

// Destination pose is defined here.
layout (location = 10) in vec4  position1;
layout (location = 11) in vec4  normal1;
layout (location = 12) in vec2  uv01;
layout (location = 13) in vec2  uv11;
#endif


#define MAX_JOINTS     64


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


layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;  
  float lod;
  int   hasJoints; 
  float w0;
  float w1;
} objBuffer;


layout (set = 3, binding = 0) uniform JointsBuffer {
  mat4 joints[MAX_JOINTS];
} jointsBuffer;


out FRAG_IN {
  vec3  position;
  float lodBias;
  vec3  normal;
  float pad1;
  vec2  uv0;
  vec2  uv1;
} frag_in;


void main()
{
  vec4 skinPosition = position;
  vec4 skinNormal = normal;
  vec2 temp_uv0 = uv0;
  vec2 temp_uv1 = uv1;
  
#if defined(INCLUDE_MORPH_TARGET_ANIMATION)
  float w0 = objBuffer.w0;
  float w1 = objBuffer.w1;
  vec3  morphPositionDiff0 = position0.xyz;
  vec3  morphPositionDiff1 = position1.xyz;
  vec3  morphNormalDiff0 = normal0.xyz;
  vec3  morphNormalDiff1 = normal1.xyz;
  vec2  morphUV00 = uv00;
  vec2  morphUV01 = uv01;
  vec2  morphUV10 = uv10;
  vec2  morphUV11 = uv11;
  skinPosition += vec4(morphPositionDiff0 * w0, 0.0);
  skinPosition += vec4(morphPositionDiff1 * w1, 0.0);
  skinNormal += vec4(morphNormalDiff0 * w0, 0.0);
  skinNormal += vec4(morphNormalDiff1 * w1, 0.0);
  temp_uv0 += morphUV00 * w0;
  temp_uv0 += morphUV01 * w1;
  temp_uv1 += morphUV10 * w0;
  temp_uv1 += morphUV11 * w1;
#endif

  // Compute the skin matrix transform.
  if (objBuffer.hasJoints >= 1) {
    mat4 skinMatrix  = jointsBuffer.joints[jointIDs[0]] * jointWeights[0];
    skinMatrix      += jointsBuffer.joints[jointIDs[1]] * jointWeights[1];
    skinMatrix      += jointsBuffer.joints[jointIDs[2]] * jointWeights[2];
    skinMatrix      += jointsBuffer.joints[jointIDs[3]] * jointWeights[3];
    
    skinPosition = skinMatrix * skinPosition;
    skinNormal = normalize(skinMatrix * skinNormal);
  }
  
  vec4 worldPosition = objBuffer.model * skinPosition;
  
  frag_in.position = worldPosition.xyz;
  frag_in.uv0 = temp_uv0;
  frag_in.uv1 = temp_uv1;
  frag_in.normal = normalize(objBuffer.normalMatrix * skinNormal).xyz;
  
  gl_Position = gWorldBuffer.viewProj * worldPosition;
}