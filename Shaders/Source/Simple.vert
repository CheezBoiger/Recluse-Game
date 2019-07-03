// Copyright (c) 2019 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (location = 0) in vec4 position;

#ifdef SKIN_ANIMATION 
#define MAX_JOINTS 64

layout (location = 4) in vec4 jointWeights;
layout (location = 5) in ivec4 jointIDs;


layout (set = 1, binding = 0) uniform JointsBuffer {
  mat4 joints[MAX_JOINTS];
} jointsBuffer;
#endif

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

layout (push_constant) uniform PushConst {
  mat4 model;
  vec4 hasJoints;
} pushInfo;


void main()
{

  vec4 worldPosition = position;
#ifdef SKIN_ANIMATION
  if (pushInfo.hasJoints.x >= 1.0) {
    mat4 skinMatrix = jointsBuffer.joints[jointIDs[0]] * jointWeights[0]
                    + jointsBuffer.joints[jointIDs[1]] * jointWeights[1]
                    + jointsBuffer.joints[jointIDs[2]] * jointWeights[2]
                    + jointsBuffer.joints[jointIDs[3]] * jointWeights[3];
    worldPosition = skinMatrix * position;
  }
#endif

  gl_Position = gWorldBuffer.global.viewProj * pushInfo.model * worldPosition;

}