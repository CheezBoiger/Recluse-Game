// Copyright (c) 2019 Recluse Project. All rights reserved.

// HDR shader.
// Reference: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/7.bloom/7.bloom_final.fs
//
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"
#include "Common/Interleaving.glsl"
#include "Common/FilmGrain.glsl"

layout (location = 0) out vec4 fragColor;

in FRAG_IN {
  vec2 position;
  vec2 uv;
} frag_in;


layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;


layout (set = 1, binding = 0) uniform sampler2D sceneSurface;
layout (set = 1, binding = 1) uniform sampler2D bloomSurface;

layout (set = 2, binding = 0) uniform HDRConfig {
  vec4 bAllowChromaticAberration;
  vec4 k;
  vec4 kcube;
  vec4 bEnable; // x = interleave, y = fade, z = filmgrain, w = distort
  vec4 interleaveShakeInterval;
  vec4 fade;
  vec4 filmGrain; // x = zoom, y = speed.
} hdr;


// TODO(): We might turn this into a descriptor set instead...
layout (push_constant) uniform ParamsHDR {
  float bloomStrength;
} paramConfigs;


vec3 Uncharted2Tonemap(vec3 x)
{
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}



void main()
{
  vec2 uv = frag_in.uv;
  vec2 resolution = vec2(gWorldBuffer.global.screenSize.xy);
  vec3 color = texture(sceneSurface, uv).rgb;
  float time = gWorldBuffer.global.fEngineTime;
  
  // TODO(): These need to be set as a descripter param instead.
  //float k = -0.15;
  //float kcube = 0.15;
  
  if (hdr.bAllowChromaticAberration.x >= 1.0) {
    float r2 = (uv.x - 0.5) * (uv.x - 0.5) + (uv.y - 0.5) * (uv.y - 0.5);
    float f = 0;
    if (hdr.kcube.x == 0.0) {
      f = 1.0 + r2 * hdr.k.x;
    } else {
      f = 1.0 + r2 * (hdr.k.x + hdr.kcube.x * sqrt(r2));
    }
    
    float ux = f*(uv.x-0.5)+0.5;
    float uy = f*(uv.y-0.5)+0.5;
    vec3 inputDistort = texture(sceneSurface, vec2(ux, uy)).rgb;
    color = vec3(inputDistort.r, color.g, color.b);
  }
  
  if (hdr.bEnable.x >= 1.0) {
    float interval = hdr.interleaveShakeInterval.x;
    vec4 interleavedColor = GenerateInterleave(sceneSurface, uv, resolution, interval, time);
    color = interleavedColor.rgb;
  }
  
  if (hdr.bEnable.z >= 1.0) {
    vec3 g = vec3(grain(uv, resolution * hdr.filmGrain.x, time * hdr.filmGrain.y));
    //float luminance = luma(color);
    //vec3 desaturated = vec3(luminance);
    color = blendSoftLight(color, g);
    //float response = smoothstep(0.05, 0.5, luminance);
    //color = mix(color, color, pow(response, 2.0));
  }
  
  // Perform an additive blending to the scene surface. This is because
  // we want to be able to enhance bloom areas within the scene texture.
  if (gWorldBuffer.global.bloomEnabled >= 1) { 
    vec3 bloom = texture(bloomSurface, uv).rgb;
    color += bloom * paramConfigs.bloomStrength; 
  }
  
  
  // Extended exposure pass with Uncharted 2 tone mapping. Gamma correction
  // is also enabled.
  color *= gWorldBuffer.global.exposure;
  vec3 tone = Uncharted2Tonemap(color);
  tone = tone * (1.0 / Uncharted2Tonemap(vec3(11.2)));
  
  // Gamma correction.
  tone = pow(tone, vec3(1.0 / gWorldBuffer.global.gamma));
  
  vec4 post = vec4(tone, 1.0);
  fragColor = post;
}






