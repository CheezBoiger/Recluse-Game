// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   texcoord0;
layout (location = 3) in vec2   texcoord1;
layout (location = 4) in vec4   jointWeights;
layout (location = 5) in ivec4  jointIDs;


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
  int   hasJoints; 
} objBuffer;


layout (set = 3, binding = 0) uniform JointsBuffer {
  mat4 joints[MAX_JOINTS];
} jointsBuffer;


out FRAG_IN {
  vec3  position;
  float lodBias;
  vec3  normal;
  float pad1;
  vec2  texcoord0;
  vec2  texcoord1;
} frag_in;


void main()
{
  vec4 skinPosition = position;
  vec4 skinNormal = normal;
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
  frag_in.texcoord0 = texcoord0;
  frag_in.texcoord1 = texcoord1;
  frag_in.normal = normalize(objBuffer.normalMatrix * skinNormal).xyz;
  
  gl_Position = gWorldBuffer.viewProj * worldPosition;
}