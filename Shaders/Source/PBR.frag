// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"
#include "Common/LightingPBR.glsl"
#include "Common/Shadowing.glsl"
#include "Common/GBufferDefines.glsl"

layout (location = 0) out vec4 vFragColor;
layout (location = 1) out vec4 vBrightColor;

in FRAG_IN {
  vec2 uv;
} frag_in;

layout (set = 1, binding = 0) uniform sampler2D rt0;
layout (set = 1, binding = 1) uniform sampler2D rt1;
layout (set = 1, binding = 2) uniform sampler2D rt2;
layout (set = 1, binding = 3) uniform sampler2D rt3;
layout (set = 1, binding = 4) uniform sampler2D rtDepth;


layout (set = 2, binding = 0) uniform LightBuffer {
  DirectionLight  primaryLight;
  DirectionLight  directionLights[MAX_DIRECTION_LIGHTS];
  PointLight      pointLights[MAX_POINT_LIGHTS];
} gLightBuffer;

layout (set = 3, binding = 0) uniform DynamicLightSpace {
  LightSpace  space;
} lightSpace;

layout (set = 3, binding = 1) uniform sampler2D dynamicShadowMap;

layout (set = 4, binding = 0) uniform StaticLightSpace {
  LightSpace space;
} staticLightSpace;

layout (set = 4, binding = 1) uniform sampler2D staticShadowMap;


struct DiffuseSH {
  vec4 c[9];
};

layout (set = 6, binding = 0) buffer GlobalMapInfo {
  DiffuseSH sh;
} globalMapInfo;
layout (set = 6, binding = 1) uniform samplerCube specMap;
layout (set = 6, binding = 2) uniform sampler2D brdfLut;
#if defined(LOCAL_REFLECTIONS)
layout (set = 6, binding = 3) buffer LocalMapInfo {
  vec4      positions[ ];
  vec4      minAABB[ ];
  vec4      maxAABB[ ];
  DiffuseSH shs[ ];
  int size;
} localMapInfo;
layout (set = 6, binding = 4) uniform samplerCubeArray specMaps;   // Current set enviroment map (radiance).
layout (set = 6, binding = 5) uniform sampler2DArray brdfLuts;    // BRDF lookup tables corresponding to each env map.
#endif


// TODO():
vec3 CookTorrBRDFPoint(PointLight light, vec3 vPosition, vec3 Albedo, vec3 V, vec3 N, float roughness, float metallic)
{
  vec3 L = light.position.xyz - vPosition;
  float distance = length(L);
  // Return if range is less than the distance between light and fragment.
  if (light.range < distance) { return vec3(0.0); }

  vec3 color = vec3(0.0);
  float falloff = (distance / light.range);
  float attenuation = light.intensity - (light.intensity * falloff);
  vec3 nV = normalize(V);
  vec3 nL = normalize(L);
  vec3 nN = normalize(N);
  
  vec3 H = normalize(nV + nL);
  
  float NoL = clamp(dot(nN, nL), 0.001, 1.0);
  float NoV = clamp(abs(dot(nN, nV)), 0.001, 1.0);
  float NoH = clamp(dot(nN, H), 0.0, 1.0);
  float VoH = clamp(dot(nV, H), 0.0, 1.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, Albedo, metallic);
  vec3 radiance = light.color.xyz * attenuation;
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, NoV, roughness);  
    vec3 F = FSchlick(VoH, F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
  
    color += (LambertDiffuse(kD, Albedo) + BRDF(D, F, G, NoL, NoV)) * NoL;
  }
  return color * radiance;
}


// TODO():
vec3 CookTorrBRDFDirectional(DirectionLight light, vec3 Albedo, vec3 V, vec3 N, float roughness, float metallic)
{
  vec3 color = vec3(0.0);
  vec3 L = -(light.direction.xyz);
  vec3 radiance = light.color.xyz * light.intensity;  
  vec3 nV = normalize(V);
  vec3 nL = normalize(L);
  vec3 nN = normalize(N);
  
  vec3 H = normalize(nV + nL);
  
  float NoL = clamp(dot(nN, nL), 0.001, 1.0);
  float NoV = clamp(abs(dot(nN, nV)), 0.001, 1.0);
  float NoH = clamp(dot(nN, H), 0.0, 1.0);
  float VoH = clamp(dot(nV, H), 0.0, 1.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, Albedo, metallic);
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, NoV, roughness);  
    vec3 F = FSchlick(VoH, F0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    color += (LambertDiffuse(kD, Albedo) + BRDF(D, F, G, NoL, NoV)) * NoL;
  }
  return color * radiance;
}


// TODO(): This will eventually be integrated into Directional, as we will need to 
// support more shadow maps (using a Sampler2DArray.)
vec3 CookTorrBRDFPrimary(DirectionLight light, vec3 vPosition, vec3 Albedo, vec3 V, vec3 N, float roughness, float metallic)
{
  vec3 color = vec3(0.0);
  vec3 L = -(light.direction.xyz);
  vec3 radiance = light.color.xyz * light.intensity;  
  vec3 nV = normalize(V);
  vec3 nL = normalize(L);
  vec3 nN = normalize(N);
  
  vec3 H = normalize(nV + nL);
  
  float NoL = clamp(dot(nN, nL), 0.001, 1.0);
  float NoV = clamp(abs(dot(nN, nV)), 0.001, 1.0);
  float NoH = clamp(dot(nN, H), 0.0, 1.0);
  float VoH = clamp(dot(nV, H), 0.0, 1.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, Albedo, metallic);
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, NoV, roughness);  
    vec3 F = FSchlick(VoH, F0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    
    color += LambertDiffuse(kD, Albedo) + BRDF(D, F, G, NoL, NoV);
    
    vec4 staticShadowClip = staticLightSpace.space.viewProj * vec4(vPosition, 1.0);
    float staticShadowFactor = ((staticLightSpace.space.shadowTechnique.x < 1) ? FilterPCF(staticShadowMap, staticShadowClip) : PCSS(staticShadowMap, staticLightSpace.space, staticShadowClip));
    float shadowFactor = staticShadowFactor;
    if (gWorldBuffer.enableShadows >= 1) {
      vec4 shadowClip = lightSpace.space.viewProj * vec4(vPosition, 1.0);
      float dynamicShadowFactor = ((lightSpace.space.shadowTechnique.x < 1) ? FilterPCF(dynamicShadowMap, shadowClip) : PCSS(dynamicShadowMap, lightSpace.space, shadowClip));
      shadowFactor = min(dynamicShadowFactor, staticShadowFactor);
    }
    
    color *= shadowFactor;
    color *= NoL;
  }
  return color * radiance;
}


void main()
{
  GBuffer gbuffer = ReadGBuffer(ivec2(frag_in.uv), rt0, rt1, rt2, rt3, rtDepth);

  vec3 N = gbuffer.normal;
  vec3 V = normalize(gWorldBuffer.cameraPos.xyz - gbuffer.pos);
  
  // Brute force lights for now.
  // TODO(): Map light probes in the future, to produce environment ambient instead.
  vec3 outColor = vec3(0.0);

  if (gLightBuffer.primaryLight.enable > 0) {
    DirectionLight light = gLightBuffer.primaryLight;
    vec3 ambient = light.ambient.rgb * gbuffer.albedo;
    outColor += ambient;
    outColor += CookTorrBRDFPrimary(light, gbuffer.pos, gbuffer.albedo, V, N, gbuffer.roughness, gbuffer.metallic); 
    outColor = max(outColor, ambient);
  }
  
  for (int i = 0; i < MAX_DIRECTION_LIGHTS; ++i) {
    DirectionLight light = gLightBuffer.directionLights[i];
    if (light.enable <= 0) { continue; }
    outColor += light.ambient.rgb * gbuffer.albedo;
    outColor += CookTorrBRDFDirectional(light, gbuffer.albedo, V, N, gbuffer.roughness, gbuffer.metallic);
  }
  
  for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
    PointLight light = gLightBuffer.pointLights[i];
    if (light.enable <= 0) { continue; }
    outColor += CookTorrBRDFPoint(light, gbuffer.pos, gbuffer.albedo, V, N, gbuffer.roughness, gbuffer.metallic);
    
  }
  
  outColor = gbuffer.emissionStrength * 20.0 * gbuffer.emission + (outColor * gbuffer.ao);
  vFragColor = vec4(outColor, 1.0);
  
  vec3 glow = outColor.rgb - length(gWorldBuffer.cameraPos.xyz - gbuffer.pos) * 0.2;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  vBrightColor = vec4(glow, 1.0);
}