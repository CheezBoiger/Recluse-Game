// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


in FRAG_IN {
  vec2 texcoord;
} frag_in;

layout (location = 0) out vec4 fragColor;


layout (binding = 0) uniform sampler2D finalTexture;


layout (set = 0, binding = 1) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  viewProj;
  vec4  cameraPos;
  vec4  l_plane;
  vec4  r_plane;
  vec4  t_plane;
  vec4  b_plane;
  vec4  n_plane;
  vec4  f_plane;
  vec2  mousePos;
  ivec2 screenSize;
  vec4  vSun; // Sundir.xyz and w is brightness.
  float gamma;
  float exposure;
  float fRayleigh;
  float fMie;
  float fMieDist;
  int   bloomEnabled;
  int   enableShadows;
  int   enableAA;
} gWorldBuffer;


// 
// TODO(): We want software AA in the final pass, not in the HDR pass!
//
#ifndef FXAA_REDUCE_MIN
    #define FXAA_REDUCE_MIN   (1.0/ 128.0)
#endif
#ifndef FXAA_REDUCE_MUL
    #define FXAA_REDUCE_MUL   (1.0 / 8.0)
#endif
#ifndef FXAA_SPAN_MAX
    #define FXAA_SPAN_MAX     8.0
#endif

vec4 fxaa(sampler2D tex, vec2 fragCoord, vec2 resolution,
  vec2 vRgbNW, vec2 vRgbNE, 
  vec2 vRgbSW, vec2 vRgbSE,
  vec2 vRgbM)
{
    vec4 color;
    vec2 inverseVP = vec2(1.0 / resolution.x, 1.0 / resolution.y);
    vec3 rgbNW = texture(tex, vRgbNW).xyz;
    vec3 rgbNE = texture(tex, vRgbNE).xyz;
    vec3 rgbSW = texture(tex, vRgbSW).xyz;
    vec3 rgbSE = texture(tex, vRgbSE).xyz;
    vec4 texColor = texture(tex, vRgbM);
    vec3 rgbM  = texColor.xyz;
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                          (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
              dir * rcpDirMin)) * inverseVP;
    
    vec3 rgbA = 0.5 * (
        texture(tex, fragCoord * inverseVP + dir * (1.0 / 3.0 - 0.5)).xyz +
        texture(tex, fragCoord * inverseVP + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, fragCoord * inverseVP + dir * -0.5).xyz +
        texture(tex, fragCoord * inverseVP + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        color = vec4(rgbA, texColor.a);
    else
        color = vec4(rgbB, texColor.a);
    return color;
}

vec4 AntiAliasingFXAA(sampler2D tex, vec2 fragCoord, ivec2 resolution)
{
	vec2 inverseVP = 1.0 / vec2(resolution);
	vec2 vRgbNW = (fragCoord + vec2(-1.0, -1.0)) * inverseVP;
	vec2 vRgbNE = (fragCoord + vec2(1.0, -1.0)) * inverseVP;
	vec2 vRgbSW = (fragCoord + vec2(-1.0, 1.0)) * inverseVP;
	vec2 vRgbSE = (fragCoord + vec2(1.0, 1.0)) * inverseVP;
	vec2 vRgbM = vec2(fragCoord * inverseVP);
  return fxaa(tex, fragCoord, vec2(resolution), vRgbNW, vRgbNE, vRgbSW, vRgbSE, vRgbM);
}


void main()
{
  vec3 final = texture(finalTexture, frag_in.texcoord).rgb;
  
  if (gWorldBuffer.enableAA >= 1) {
    vec2 aaCoord = frag_in.texcoord * vec2(gWorldBuffer.screenSize);
    final = AntiAliasingFXAA(finalTexture, aaCoord, gWorldBuffer.screenSize).rgb;
  }

  vec4 color = vec4(final, 1.0);
  fragColor = color;
}