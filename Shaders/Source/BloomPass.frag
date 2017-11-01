// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 outColor;

in FRAG_IN {
  vec2 uv;
} frag_in;

// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  viewProj;
  vec4  cameraPos;
  float coffSH[9];
  ivec2 screenSize;
  int  pad;
} gWorldBuffer;

layout (set = 0, binding = 1) uniform sampler2D sceneSurface;

// We want to use the normal in order to better approximate a bloom effect
// on the needed surface.
layout (set = 0, binding = 2) uniform sampler2D normalSurface;

// TODO(): Emissive map may be wanted in order to add bloom.

void main()
{
  vec3 blurColor = vec3(0.0);
  
  // TODO(): Perform blur.
  
  outColor = vec4(blurColor, 1.0);
}