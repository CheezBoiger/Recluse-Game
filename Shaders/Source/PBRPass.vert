// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   texcoord0;
layout (location = 3) in vec2   texcoord1;
layout (location = 4) in vec4   color;
layout (location = 5) in vec4   boneWeights;
layout (location = 6) in ivec4  boneIDs;


#define MAX_BONES     64


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
  ivec2 screenSize;
  ivec2 pad;
} gWorldBuffer;


layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  int   hasAlbedo;
  int   hasMetallic;
  int   hasRoughness;
  int   hasNormal;
  int   hasEmissive;
  int   hasAO;
  int   hasBones; 
  int   pad;
} objBuffer;


layout (set = 1, binding = 1) uniform BonesBuffer {
  mat4 bones[MAX_BONES];
} boneBuffer;


layout (location = 0) out vec4 fragPos;
layout (location = 1) out vec4 fragNormal;
layout (location = 2) out vec2 fragTexCoord;


out FRAG_IN {
  vec3  position;
  float pad0;
  vec3  normal;
  float pad1;
  vec2  texcoord0;
  vec2  texcoord1;
  vec4  color;
} frag_in;


void main()
{
  vec4 worldPosition = position;
 
  // Compute the bone transform 
  if (objBuffer.hasBones >= 1) {
    mat4 boneTransform  = boneBuffer.bones[boneIDs[0]] * boneWeights[0];
    boneTransform      += boneBuffer.bones[boneIDs[1]] * boneWeights[1];
    boneTransform      += boneBuffer.bones[boneIDs[2]] * boneWeights[2];
    boneTransform      += boneBuffer.bones[boneIDs[3]] * boneWeights[3];
    
    worldPosition = boneTransform * worldPosition;
  }
  
  worldPosition = objBuffer.model * worldPosition;
  
  frag_in.position = worldPosition.xyz;
  frag_in.texcoord0 = texcoord0;
  frag_in.texcoord1 = texcoord1;
  frag_in.color = color;
  frag_in.normal = normalize(objBuffer.normalMatrix * normal).xyz;
  
  gl_Position = gWorldBuffer.viewProj * worldPosition;
}