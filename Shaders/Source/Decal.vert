// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

// Bounding box and it's uv coordinates. No normal is required as it only defines 
// the bounds of the decal.
layout (location = 0) in vec4 position;
layout (location = 1) in vec2 uv0;
layout (location = 2) in vec2 uv1;

layout (push_constant) uniform DecalTransform {
  mat4      model;
  mat4      invModel;
  float     lodBias;
  float     opacity;
} transform;


out FRAG_IN {
  vec3  positionCS;   // position of boundingbox in clip space.
  float lodBias;      // level of detail.
  vec2  uv0;          // uv coords.
  vec2  uv1;          // second uv coords.
  vec4  opacity;      // opacity.
} fragIn;

void main()
{
  fragIn.lodBias = transform.lodBias;
  fragIn.opacity.x = transform.opacity;
  fragIn.uv0 = uv0;
  fragIn.uv1 = uv1;
  
  vec4 clipPos = gWorldBuffer.viewProj * transform.model * position;
  gl_Position = clipPos;
  fragIn.positionCS = clipPos.xyz;
}