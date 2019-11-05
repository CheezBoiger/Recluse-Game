// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (location = 0) in vec4 position;

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

layout (push_constant) uniform SkyBoxParams {
  mat4 modelToWorld;
} skyboxTransform;

out FRAG_IN {
  vec3 uvw;
} frag_in;

void main()
{
  frag_in.uvw = position.xyz;
  mat4 modelToWorld = skyboxTransform.modelToWorld;
  mat4 view = mat4(mat3(gWorldBuffer.global.view));
  vec4 pos = gWorldBuffer.global.proj * view * modelToWorld * position;
  gl_Position = pos.xyww;
}