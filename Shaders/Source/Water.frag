// Copyright (c) 2018 Recluse Project. All rights reserved.
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 1, location = 0) sampler2D flowMap;
layout (set = 1, location = 1) sampler2D normalMap;
layout (set = 1, location = 2) sampler2D refractMap;
layout (set = 1, location = 3) sampler2D reflectMap;


in FragIn {
  vec2 uv0;
  vec2 uv1;
  vec4 clipPos;
} frag_in;


void main() 
{
  
}