// Copyright (c) 2019 Recluse Project. All rights reserved.
#ifndef FILMGRAIN_GLSL
#define FILMGRAIN_GLSL

#include "Noise.glsl"

// Implemenation from: https://github.com/mattdesl/glsl-film-grain
// Luma values from https://github.com/hughsk/glsl-luma/blob/master/index.glsl


float grain(vec2 texCoord, vec2 resolution, float frame, float multiplier) 
{
    vec2 mult = texCoord * resolution;
    float offset = snoise3(vec3(mult / multiplier, frame));
    float n1 = pnoise(vec3(mult, offset), vec3(1.0/texCoord * resolution, 1.0));
    return n1 / 2.0 + 0.5;
}


float grain(vec2 texCoord, vec2 resolution, float frame) 
{
    return grain(texCoord, resolution, frame, 2.5);
}


float grain(vec2 texCoord, vec2 resolution) 
{
    return grain(texCoord, resolution, 0.0);
}


vec3 blendSoftLight(vec3 base, vec3 blend) 
{
    return mix(
        sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend), 
        2.0 * base * blend + base * base * (1.0 - 2.0 * blend), 
        step(base, vec3(0.5))
    );
}


float luma(vec3 color) 
{
  return dot(color, vec3(0.299, 0.587, 0.114));
}

float luma(vec4 color) 
{
  return dot(color.rgb, vec3(0.299, 0.587, 0.114));
}


#endif // FILMGRAIN_GLSL