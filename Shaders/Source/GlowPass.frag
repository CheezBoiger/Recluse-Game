// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 GlowColor;


layout (set = 0, binding = 0) uniform sampler2D Target2x;
layout (set = 0, binding = 1) uniform sampler2D Target4x;
layout (set = 0, binding = 2) uniform sampler2D Target8x;


in FRAG_IN {
  vec2 uv;
} frag_in;


void main()
{
  vec3 t2x = texture(Target2x, frag_in.uv).rgb;
  vec3 t4x = texture(Target4x, frag_in.uv).rgb;
  vec3 t8x = texture(Target8x, frag_in.uv).rgb;
  
  vec3 Combined = t2x + t4x;
  Combined = Combined + t8x;
  
  GlowColor = vec4(Combined, 1.0);
}