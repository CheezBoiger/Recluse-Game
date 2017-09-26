// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

in FRAG_IN {
  vec4 position;
  vec4 normal;
  vec2 texcoord0;
  vec2 texcoord1;
  vec4 color;
} frag_in;

#define MAX_LIGHTS            512

struct DirectionLight {
  vec4 direction;
  vec4 color;
};


struct PointLight {
  vec4  position;
  vec4  color;
  float range;
  float pad0[3];
};


// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  viewProj;
  mat4  cameraView;
  mat4  cameraProj;
  vec4  cameraPos;
  float coffSH[9];
  float pad1[3];
} gWorldBuffer;


layout (set = 1, binding = 2) uniform sampler2D albedo;
layout (set = 1, binding = 3) uniform sampler2D metallic;
layout (set = 1, binding = 4) uniform sampler2D roughness; 


layout (set = 2, binding = 0) uniform LightBuffer {
  DirectionLight  primaryLight;
  PointLight      pointLights[MAX_LIGHTS];
} gLightBuffer;


// TODO(): Need to addin gridLights buffer, for light culling, too.

layout (location = 0) out vec4 OutColor;


void main()
{
  vec4 fragAlbedo = texture(albedo, frag_in.texcoord0);
  vec4 fragMetallic = texture(metallic, frag_in.texcoord0);
  vec4 fragRoughness = texture(roughness, frag_in.texcoord0);
  
  vec4 V = normalize(gWorldBuffer.cameraPos - frag_in.position);
  vec4 N = normalize(frag_in.normal);
  
  OutColor = vec4(0.0, 0.0, 0.0, 1.0);
}