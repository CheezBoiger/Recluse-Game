// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (location = 0) out vec4 vFragColor;
layout (location = 1) out vec4 vBrightColor;

in FRAG_IN {
  vec3 uvw;
} frag_in;

layout (set = 1, binding = 0) uniform samplerCube skyTexture;

void main()
{
  vFragColor = texture(skyTexture, frag_in.uvw);
 
  vBrightColor = vec4(0.0, 0.0, 0.0, 1.0);
  // We might just want to work with the final image.
  vec3 pos = normalize(frag_in.uvw);
  float bright = dot(pos, -gWorldBuffer.vSun.xyz);

  vec3 glow = vFragColor.rgb * bright;
  glow = max(glow, vec3(0.0));
  glow = glow * gWorldBuffer.vSun.w;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  vBrightColor = vec4(glow, 1.0);
}