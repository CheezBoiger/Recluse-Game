// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 outColor;

in FRAG_IN {
  vec2 uv;
} frag_in;


// The surface we are blurring.
layout (set = 0, binding = 0) uniform sampler2D sceneSurface;

// Push constants to determine direction of blur.
layout (push_constant) uniform Consts {
  int   horizontal;
  float strength;
} Blur;


void main()
{
  // Gaussian weights
  float weight[5];
  weight[0] = 0.2270270270;
  weight[1] = 0.1945945946;
  weight[2] = 0.1216216216;
  weight[3] = 0.0540540541;
  weight[4] = 0.0162162162;
  
  vec2 offset = 1.0 / textureSize(sceneSurface, 0);
  vec3 blurColor = texture(sceneSurface, frag_in.uv).rgb * weight[0];
  
  // TODO(): Perform blur. May want to do a liner sample instead?
  if (Blur.horizontal == 0) {
    for (int i = 1; i < 5; ++i) {
      blurColor += texture(sceneSurface, frag_in.uv + vec2(offset.x * i, 0.0)).rgb * weight[i] * Blur.strength;
      blurColor += texture(sceneSurface, frag_in.uv - vec2(offset.x * i, 0.0)).rgb * weight[i] * Blur.strength;
    }
  } else {
    for (int i = 1; i < 5; ++i) {
      blurColor += texture(sceneSurface, frag_in.uv + vec2(0.0, offset.y * i)).rgb * weight[i] * Blur.strength;
      blurColor += texture(sceneSurface, frag_in.uv - vec2(0.0, offset.y * i)).rgb * weight[i] * Blur.strength;
    }
  }
  
  outColor = vec4(blurColor, 1.0);
}