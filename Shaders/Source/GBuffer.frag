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

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

layout (set = 1, binding = 0) uniform ObjectBuffer {
  Model obj;
} objBuffer;


layout (set = 2, binding = 0) uniform MaterialBuffer {
  Material material;
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
  vec2 offsetUV0 = matBuffer.material.offsetUV.xy;
  vec2 uv0 = frag_in.texcoord0 + offsetUV0;
  vec3 fragAlbedo = matBuffer.material.color.rgb;
  vec3 fragNormal = frag_in.normal;
  vec3 fragEmissive = vec3(0.0);

  float fragMetallic  = matBuffer.material.metal;
  float fragRoughness = matBuffer.material.rough;
  float fragAO = 1.0;  // still WIP
  
  if (matBuffer.material.hasAlbedo >= 1) {
    fragAlbedo = pow(texture(albedo, uv0, objBuffer.obj.lod).rgb, vec3(2.2));
  }
  
  if (matBuffer.material.hasMetallic >= 1 || matBuffer.material.hasRoughness >= 1) {
    vec4 roughMetal = texture(roughnessMetallic, uv0, objBuffer.obj.lod);
    if (matBuffer.material.hasMetallic >= 1) {
      fragMetallic *= roughMetal.b;
    }
  
    if (matBuffer.material.hasRoughness >= 1) {
      fragRoughness *= clamp(roughMetal.g, 0.04, 1.0);
    }
  }
  
  if (matBuffer.material.hasNormal >= 1) {
    fragNormal = GetNormal(normal, objBuffer.obj.lod, frag_in.normal, frag_in.position, uv0);
  }
  
  if (matBuffer.material.hasAO >= 1) {
    fragAO = texture(ao, uv0, objBuffer.obj.lod).r;
  }
  
  if (matBuffer.material.hasEmissive >= 1) {
    fragEmissive = pow(texture(emissive, uv0, objBuffer.obj.lod).rgb, vec3(2.2));
  }   
  
  vec3 N = normalize(fragNormal);
  
#ifndef _DEBUG
  GBuffer gbuffer;
  gbuffer.albedo = fragAlbedo;
  gbuffer.pos = frag_in.position;
  gbuffer.normal = N;
  gbuffer.emission = fragEmissive;
  gbuffer.emissionStrength = matBuffer.material.emissive;
  gbuffer.metallic = fragMetallic;
  gbuffer.roughness = fragRoughness;
  gbuffer.ao = fragAO;
  gbuffer.anisoSpec = matBuffer.material.anisoSpec;
  gbuffer.anisoSpec.z = frag_in.vpos.z;
  
  WriteGBuffer(gbuffer, rt0, rt1, rt2, rt3);
#else
  debugColor = vec4(fragAlbedo, 1.0);
#endif
}