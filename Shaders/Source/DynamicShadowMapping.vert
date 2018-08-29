// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable

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

layout (set = 0, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  float lod;
  int   hasJoints; 
  float w0;
  float w1;
} obj_buffer;


// One directional light.
layout (set = 1, binding = 0) uniform LightSpace {
  mat4  viewProj;
} light_space;


layout (set = 3, binding = 0) uniform JointsBuffer {
  mat4 joints[MAX_JOINTS];
} joints_buffer;


out FragIn {
  vec2 uv0;
  vec2 uv1;
} fragIn;


void main()
{
  vec4 worldPosition = position;
  vec2 temp_uv0 = uv0;
  vec2 temp_uv1 = uv1;
  
#if defined(INCLUDE_MORPH_TARGET_ANIMATION)
  float w0 = objBuffer.w0;
  float w1 = objBuffer.w1;
  float wC = 1.0 - w0 - w1;
  clamp(wC, 0.0, 1.0);
  float wS = w0 + w1 + wC;
  float mtf0 = w0 / wS;
  float mtf1 = w1 / wS;
  float mtfC = wC / wS;
  // Interpolate between two target animations, using weights to define the current extremes.
  worldPosition = mtf0 * position0 + mtf1 * position1 + mtfC * position; 
  temp_uv0 = mtf0 * uv00 + mtf1 * uv01 + mtfC * uv0;
  temp_uv1 = mtf0 * uv10 + mtf1 * uv11 + mtfC * uv1;
#endif
 
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
  
  fragIn.uv0 = temp_uv0;
  fragIn.uv1 = temp_uv1;
}