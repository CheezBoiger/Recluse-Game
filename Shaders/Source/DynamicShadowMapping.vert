// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   texcoord0;
layout (location = 3) in vec2   texcoord1;
layout (location = 4) in vec4   jointWeights;
layout (location = 5) in ivec4  jointIDs;

#define MAX_JOINTS     64

layout (set = 0, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  float lod;
  int   hasJoints; 
} obj_buffer;


// One directional light.
layout (set = 1, binding = 0) uniform LightSpace {
  mat4  viewProj;
} light_space;


layout (set = 2, binding = 0) uniform JointsBuffer {
  mat4 joints[MAX_JOINTS];
} joints_buffer;


#if defined(TRANSPARENT_SHADOW_MAP)
out FragIn {
  vec2 uv0;
  vec2 uv1;
} fragIn;
#endif


void main()
{
  vec4 worldPosition = position;
 
  // Compute the skin matrix.
  if (obj_buffer.hasJoints >= 1) {
    mat4 skinMatrix  = joints_buffer.joints[jointIDs[0]] * jointWeights[0];
    skinMatrix      += joints_buffer.joints[jointIDs[1]] * jointWeights[1];
    skinMatrix      += joints_buffer.joints[jointIDs[2]] * jointWeights[2];
    skinMatrix      += joints_buffer.joints[jointIDs[3]] * jointWeights[3];
    
    worldPosition = skinMatrix * worldPosition;
  }
  
  worldPosition = obj_buffer.model * worldPosition;
  gl_Position = light_space.viewProj * worldPosition;
  
#if defined(TRANSPARENT_SHADOW_MAP)
  fragIn.uv0 = texcoord0;
  fragIn.uv1 = texcoord1;
#endif
}