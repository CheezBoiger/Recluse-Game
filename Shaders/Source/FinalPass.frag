// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


in FragIn {
  vec2 uv;
} frag_in;

layout (location = 0) out vec4 fragColor;


layout (binding = 0) uniform sampler2D finalTexture;

void main()
{
  vec3 final = texture(finalTexture, frag_in.uv).rgb;
  vec4 color = vec4(final, 1.0);
  fragColor = color;
}