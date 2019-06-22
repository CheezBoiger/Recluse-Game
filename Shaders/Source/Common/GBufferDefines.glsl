// Copyright (c) 2018 Recluse Project. All rights reserved.
#ifndef GBUFFERDEFINES_H
#define GBUFFERDEFINES_H

#include "Globals.glsl"

#define NAN                     0./0.

struct GBuffer
{
  vec3 albedo;
  vec3 normal;
  vec3 pos;
  vec3 emission;
  float emissionStrength;
  float roughness;
  float metallic;
  float ao;
  vec4 anisoSpec;
};


GBuffer ReadGBuffer(ivec2 uv, 
          in GlobalBuffer global,
					in sampler2D inRt0,
					in sampler2D inRt1, 
					in sampler2D inRt2,
					in sampler2D inRt3,
          in sampler2D inDepth)
{
  GBuffer gbuffer;
  vec4 albedo = texelFetch(inRt0, uv, 0);
  vec4 normal = texelFetch(inRt1, uv, 0);
  vec4 erm    = texelFetch(inRt2, uv, 0);
  vec4 emit   = texelFetch(inRt3, uv, 0);
  
  gbuffer.albedo            = albedo.rgb;
  gbuffer.normal            = normal.rgb * 2.0 - 1.0;
  gbuffer.emission          = emit.rgb;
  gbuffer.emissionStrength  = erm.r;
  gbuffer.roughness         = erm.g;
  gbuffer.metallic          = erm.b;
  gbuffer.ao                = albedo.a;
  gbuffer.anisoSpec         = vec4(normal.a, erm.a, 0.0, 0.0);
  
  vec2 resolution = vec2(global.screenSize);
  float z = texelFetch(inDepth, uv, 0).r;
  float x = float((uv.x / resolution.x) * 2 - 1);
  float y = float((uv.y / resolution.y) * 2 - 1);
  vec4 clipPos = vec4(x, y, z, 1.0);
  vec4 worldPos = global.invViewProj * clipPos;
  worldPos /= worldPos.w;
  gbuffer.pos = worldPos.xyz;

  return gbuffer;
}


void WriteGBuffer(GBuffer gbuffer, 
                  inout vec4 rt0,
                  inout vec4 rt1,
                  inout vec4 rt2,
                  inout vec4 rt3)
{
  rt0 = vec4(gbuffer.albedo, gbuffer.ao);
  rt1 = vec4(gbuffer.normal * 0.5 + 0.5, gbuffer.anisoSpec.x);
  rt2 = vec4(gbuffer.emissionStrength, gbuffer.roughness, gbuffer.metallic, gbuffer.anisoSpec.y);
  rt3 = vec4(gbuffer.emission, 0.0);
}


float linearizeDepth(in sampler2D depthTex, vec2 vTexCoord, float zNear, float zFar)
{
  float zB = texture(depthTex, vTexCoord).z;
  float zN = 2.0 * zB - 1.0;
  float zE = 2.0 * zNear * zFar / (zFar + zNear - zN * (zFar - zNear));
  return zE;
}

#endif // GBUFFERDEFINES_H