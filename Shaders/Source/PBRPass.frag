// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 fragPos;
layout (location = 1) in vec4 fragNormal;
layout (location = 2) in vec2 fragTexCoord;

#define MAX_BONES             64
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


layout (binding = 0) uniform GlobalBuffer {
  mat4  model;
  mat4  view;
  mat4  proj;
  mat4  viewProj;
  mat4  modelViewProj;
  mat4  inverseNormalMatrix;
  mat4  cameraView;
  mat4  cameraProj;
  mat4  bones[MAX_BONES];
  vec4  cameraPos;
  float hasAlbedo;
  float hasMetallic;
  float hasRoughness;
  float pad0;
  float coffSH[9];
  float pad1[3];
} gWorldBuffer;


layout (binding = 1) uniform LightBuffer {
  DirectionLight  primaryLight;
  PointLight      pointLights[MAX_LIGHTS];
};


layout (binding = 2) uniform sampler2D albedo;
layout (binding = 3) uniform sampler2D metallic;
layout (binding = 4) uniform sampler2D roughness; 

// TODO(): Need to addin gridLights buffer, for light culling, too.

layout (location = 0) out vec4 OutColor;


void main()
{
  vec4 fragAlbedo = texture(albedo, fragTexCoord);
  vec4 fragMetallic = texture(metallic, fragTexCoord);
  vec4 fragRoughness = texture(roughness, fragTexCoord);
  
  vec4 V = normalize(gWorldBuffer.cameraPos - fragPos);
  vec4 N = normalize(fragNormal);
  
  OutColor = vec4(0.0, 0.0, 0.0, 1.0);
}