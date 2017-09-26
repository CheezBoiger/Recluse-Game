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


layout (binding = 0) uniform GlobalBuffer {
  mat4  model;
  mat4  view;
  mat4  proj;
  mat4  viewProj;
  mat4  modelViewProj;
  mat4  inverseNormalMatrix;
  mat4  cameraView;
  mat4  cameraProj;
  mat4  bones[MAX_BONES];
  vec4  cameraPos;
  float hasAlbedo;
  float hasMetallic;
  float hasRoughness;
  float pad0;
  float coffSH[9];
  float pad1[3];
} gWorldBuffer;


layout (location = 0) out vec4 fragPos;
layout (location = 1) out vec4 fragNormal;
layout (location = 2) out vec2 fragTexCoord;


out FRAG_IN {
  vec4 position;
  vec4 normal;
  vec2 texcoord0;
  vec2 texcoord1;
  vec4 color;
} frag_in;


void main()
{
  // Compute the bone transform 
  mat4 boneTransform  = gWorldBuffer.bones[boneIDs[0]] * boneWeights[0];
  boneTransform      += gWorldBuffer.bones[boneIDs[1]] * boneWeights[1];
  boneTransform      += gWorldBuffer.bones[boneIDs[2]] * boneWeights[2];
  boneTransform      += gWorldBuffer.bones[boneIDs[3]] * boneWeights[3];
  
  vec4 worldPosition = gWorldBuffer.model * boneTransform * position;
  
  frag_in.position = worldPosition;
  frag_in.normal = gWorldBuffer.inverseNormalMatrix * normal;
  frag_in.texcoord0 = texcoord0;
  frag_in.texcoord1 = texcoord1;
  frag_in.color = color;
  
  gl_Position = gWorldBuffer.modelViewProj * position;
}