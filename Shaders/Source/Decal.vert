// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

// Bounding box position vertices. No normals or uvs are required as it only defines 
// the bounds of the decal.
layout (location = 0) in vec4 position;

layout (push_constant) uniform DecalTransform {
  mat4      obb;
  mat4      invObb;
  float     lodBias;
  float     opacity;
} transform;


out FRAG_IN {
  vec4  posCS; // position clip space.
  vec4  opacity;      // opacity.
  vec4 lodBias;      // level of detail.
} fragIn;

void main()
{
  fragIn.lodBias = vec4(transform.lodBias).xxxx;
  fragIn.opacity.x = transform.opacity;
  
  vec4 clipPos = gWorldBuffer.global.viewProj * transform.obb * position;
  gl_Position = clipPos;
  fragIn.posCS = clipPos;
}