// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   texcoord0;
layout (location = 3) in vec2   texcoord1;
layout (location = 4) in vec4   boneWeights;
layout (location = 5) in ivec4  boneIDs;

#define MAX_BONES     64

layout (set = 0, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  int   hasBones; 
} obj_buffer;


// One directional light.
layout (set = 1, binding = 0) uniform LightSpace {
  mat4  viewProj;
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
  gl_Position = light_space.viewProj * worldPosition;
}