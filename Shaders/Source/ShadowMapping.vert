// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable

layout (location = 0) in vec4 position;


layout (set = 0, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  vec4  color;
  float levelOfDetail;
  float transparency;
  float metal;
  float rough;
  int   hasAlbedo;
  int   hasMetallic;
  int   hasRoughness;
  int   hasNormal;
  int   hasEmissive;
  int   hasAO;
  int   hasBones; 
  int   isTransparent;
} obj_buffer;


// One directional light.
layout (set = 1, binding = 0) uniform LightSpace {
  mat4  proj;
  mat4  view;
} light_space;


void main()
{
  mat4 mvp = light_space.proj * light_space.view * obj_buffer.model;
  gl_Position = mvp * vec4(position.xyz, 1.0);
}