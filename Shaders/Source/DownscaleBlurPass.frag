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
  float scale;
} Blur;


void main()
{
  // Gaussian weights
  float weight[5];
  weight[0] = 70.0 / 256.0;
  weight[1] = 56.0 / 256.0;
  weight[2] = 28.0 / 256.0;
  weight[3] =  8.0 / 256.0;
  weight[4] =  1.0 / 256.0;
  
  vec2 offset = 1.0 / textureSize(sceneSurface, 0) * Blur.scale;
  vec3 blurColor = texture(sceneSurface, frag_in.uv).rgb * weight[0];
  
  // TODO(): Perform blur. May want to do a linear sample instead?
  // <-------- coefficients --------->  for horizontal.
  if (Blur.horizontal == 1) {
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