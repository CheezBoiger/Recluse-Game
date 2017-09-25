// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_languange_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;


layout (location = 1) out vec2 fragTexCoord;


void main() 
{
  gl_Position = vec4(position, 0.0, 1.0);
  fragTexCoord  = texcoord;
}