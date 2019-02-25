// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"
#include "Common/GBufferDefines.glsl"
#include "Common/LightingPBR.glsl"

in FRAG_IN {
  vec3  position;
  float lodBias;
  vec3  normal;
  float pad1;
  vec2  texcoord0;
  vec2  texcoord1;
  vec4  vpos;
} frag_in;

layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  float lod;          // Level of Detail
  int   hasBones; 
  float w0;
  float w1;
} objBuffer;


layout (set = 2, binding = 0) uniform MaterialBuffer {
  vec4  color;
  vec4  anisoSpec;
  vec4  offsetUV;
  float opaque;
  float metal;
  float rough;
  float emissive;
  int   hasAlbedo;
  int   hasMetallic;
  int   hasRoughness;
  int   hasNormal;
  int   hasEmissive;
  int   hasAO;
  int   isTransparent;
  int   pad;
} matBuffer;


layout (set = 2, binding = 1) uniform sampler2D albedo;
layout (set = 2, binding = 2) uniform sampler2D roughnessMetallic;
layout (set = 2, binding = 3) uniform sampler2D normal;
layout (set = 2, binding = 4) uniform sampler2D ao;
layout (set = 2, binding = 5) uniform sampler2D emissive;

#ifndef _DEBUG
layout (location = 0) out vec4 rt0;
layout (location = 1) out vec4 rt1;
layout (location = 2) out vec4 rt2;
layout (location = 3) out vec4 rt3;
#else
//
//
// 
//
layout (push_constant) uniform Debug {
  ivec4 config;
} kDebug;

layout (location = 0) out vec4 debugColor;
#endif

void main()
{ 
  vec2 offsetUV0 = matBuffer.offsetUV.xy;
  vec2 uv0 = frag_in.texcoord0 + offsetUV0;
  vec3 fragAlbedo = matBuffer.color.rgb;
  vec3 fragNormal = frag_in.normal;
  vec3 fragEmissive = vec3(0.0);

  float fragMetallic  = matBuffer.metal;
  float fragRoughness = matBuffer.rough;
  float fragAO = 1.0;  // still WIP
  
  if (matBuffer.hasAlbedo >= 1) {
    fragAlbedo = pow(texture(albedo, uv0, objBuffer.lod).rgb, vec3(2.2));
  }
  
  if (matBuffer.hasMetallic >= 1 || matBuffer.hasRoughness >= 1) {
    vec4 roughMetal = texture(roughnessMetallic, uv0, objBuffer.lod);
    if (matBuffer.hasMetallic >= 1) {
      fragMetallic *= roughMetal.b;
    }
  
    if (matBuffer.hasRoughness >= 1) {
      fragRoughness *= clamp(roughMetal.g, 0.04, 1.0);
    }
  }
  
  if (matBuffer.hasNormal >= 1) {
    fragNormal = GetNormal(normal, objBuffer.lod, frag_in.normal, frag_in.position, uv0);
  }
  
  if (matBuffer.hasAO >= 1) {
    fragAO = texture(ao, uv0, objBuffer.lod).r;
  }
  
  if (matBuffer.hasEmissive >= 1) {
    fragEmissive = pow(texture(emissive, uv0, objBuffer.lod).rgb, vec3(2.2));
  }   
  
  vec3 N = normalize(fragNormal);
  
#ifndef _DEBUG
  GBuffer gbuffer;
  gbuffer.albedo = fragAlbedo;
  gbuffer.pos = frag_in.position;
  gbuffer.normal = N;
  gbuffer.emission = fragEmissive;
  gbuffer.emissionStrength = matBuffer.emissive;
  gbuffer.metallic = fragMetallic;
  gbuffer.roughness = fragRoughness;
  gbuffer.ao = fragAO;
  gbuffer.anisoSpec = matBuffer.anisoSpec;
  gbuffer.anisoSpec.z = frag_in.vpos.z;
  
  WriteGBuffer(gbuffer, rt0, rt1, rt2, rt3);
#else
  debugColor = vec4(fragAlbedo, 1.0);
#endif
}