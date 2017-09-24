#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texcoord;


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


layout (location = 0) out vec4 fragPos;
layout (location = 1) out vec4 fragNormal;
layout (location = 2) out vec2 fragTexCoord;


void main()
{
  vec4 worldPosition = gWorldBuffer.model * position;
  fragPos = worldPosition;
  fragNormal = gWorldBuffer.inverseNormalMatrix * normal;
  fragTexCoord = texcoord;
  
  gl_Position = gWorldBuffer.modelViewProj * position;
}