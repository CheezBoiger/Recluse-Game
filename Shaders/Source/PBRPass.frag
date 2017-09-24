#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 fragPos;
layout (location = 1) in vec4 fragNormal;
layout (location = 2) in vec2 fragTexCoord;


layout (binding = 0) uniform GlobalBuffer {
  mat4 model;
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  mat4 modelViewProj;
  mat4 inverseNormalMatrix;
  mat4 cameraView;
  mat4 cameraProj;
  vec4 cameraPos;
} gWorldBuffer;


layout (binding = 1) uniform sampler2D albedo;
layout (binding = 2) uniform sampler2D metallic;
layout (binding = 3) uniform sampler2D roughness; 

// TODO(): Need to addin gridLights buffer, for light culling, too.

layout (location = 0) out vec4 OutColor;


void main()
{
  OutColor = vec4(0.0, 0.0, 0.0, 1.0);
}