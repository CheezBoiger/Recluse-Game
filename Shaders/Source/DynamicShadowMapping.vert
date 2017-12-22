// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   texcoord0;
layout (location = 3) in vec2   texcoord1;
layout (location = 5) in vec4   boneWeights;
layout (location = 6) in ivec4  boneIDs;

#define MAX_BONES     64

layout (set = 0, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  vec4  color;
  float levelOfDetail;
  float transparency;
  float metal;
  float rough;
  float emissive;
  int   hasAlbedo;
  int   hasMetallic;
  int   hasRoughness;
  int   hasNormal;
  int   hasEmissive;
  int   hasAO;
  int   hasBones; 
  int   isTransparent;
  // Needs to be padded 3 ints.
} obj_buffer;


// One directional light.
layout (set = 1, binding = 0) uniform LightSpace {
  mat4  proj;
  mat4  view;
} light_space;


layout (set = 2, binding = 0) uniform BonesBuffer {
  mat4 bones[MAX_BONES];
} bone_buffer;


void main()
{
  vec4 worldPosition = position;
 
  // Compute the bone transform 
  if (obj_buffer.hasBones >= 1) {
    mat4 boneTransform  = bone_buffer.bones[boneIDs[0]] * boneWeights[0];
    boneTransform      += bone_buffer.bones[boneIDs[1]] * boneWeights[1];
    boneTransform      += bone_buffer.bones[boneIDs[2]] * boneWeights[2];
    boneTransform      += bone_buffer.bones[boneIDs[3]] * boneWeights[3];
    
    worldPosition = boneTransform * worldPosition;
  }
  
  worldPosition = obj_buffer.model * worldPosition;
  gl_Position = light_space.proj * light_space.view * worldPosition;
}