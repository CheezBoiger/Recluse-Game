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

layout (set = 1, binding = 0) uniform sampler2D rt0;
layout (set = 1, binding = 1) uniform sampler2D rt1;
layout (set = 1, binding = 2) uniform sampler2D rt2;
layout (set = 1, binding = 3) uniform sampler2D rt3;
layout (set = 1, binding = 4) uniform sampler2D rtDepth;


layout (set = 2, binding = 0) uniform LightBuffer {
  DirectionLight  primaryLight;
  DirectionLight  directionLights[MAX_DIRECTION_LIGHTS];
  PointLight      pointLights[MAX_POINT_LIGHTS];
  SpotLight       spotLights[MAX_SPOT_LIGHTS];
} gLightBuffer;

layout (set = 3, binding = 0) uniform DynamicLightSpace {
  LightSpaceCascade  lightSpace;
} dynamicLightSpace;

layout (set = 3, binding = 1) uniform sampler2DArray dynamicShadowMap;
/*
layout (set = 4, binding = 0) uniform StaticLightSpace {
  LightSpace lightSpace;
} staticLightSpace;

layout (set = 4, binding = 1) uniform sampler2D staticShadowMap;
*/
layout (set = 4, binding = 0) buffer GlobalMapInfo {
  DiffuseSH sh;
} globalMapInfo;
layout (set = 4, binding = 1) uniform samplerCube specMap;
layout (set = 4, binding = 2) uniform sampler2D brdfLut;
#if defined(LOCAL_REFLECTIONS)
layout (set = 4, binding = 3) buffer LocalMapInfo {
  vec4      positions[32];
  vec4      minAABB[32];
  vec4      maxAABB[32];
  DiffuseSH shs[32];
} localMapInfo;
layout (set = 4, binding = 4) uniform samplerCubeArray specMaps;   // Current set enviroment map (radiance).
layout (set = 4, binding = 5) uniform sampler2DArray brdfLuts;    // BRDF lookup tables corresponding to each env map.
#endif


void main()
{
  GBuffer gbuffer = ReadGBuffer(ivec2(gl_FragCoord.xy), rt0, rt1, rt2, rt3, rtDepth);

  vec3 N = normalize(gbuffer.normal);
  vec3 V = normalize(gWorldBuffer.cameraPos.xyz - gbuffer.pos);
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, gbuffer.albedo, gbuffer.metallic);
  float NoV = clamp(abs(dot(N, V)), 0.001, 1.0);
  vec3 R = -normalize(V - 2.0 * dot(N, V) * N);

  PBRInfo pbrInfo;
  pbrInfo.WP = gbuffer.pos;
  pbrInfo.albedo = gbuffer.albedo;
  pbrInfo.F0 = F0;
  pbrInfo.NoV = NoV;
  pbrInfo.N = N;
  pbrInfo.V = V;
  pbrInfo.roughness = gbuffer.roughness;
  pbrInfo.metallic = gbuffer.metallic;
  
  // Brute force lights for now.
  // TODO(): Map light probes in the future, to produce environment ambient instead.
  vec3 outColor = GetIBLContribution(pbrInfo, R, brdfLut, globalMapInfo.sh, specMap);//vec3(0.0);

  if (gLightBuffer.primaryLight.enable > 0) {
    DirectionLight light = gLightBuffer.primaryLight;
    vec3 ambient = light.ambient.rgb * gbuffer.albedo;
    vec3 radiance = CookTorrBRDFDirectional(light, pbrInfo);
    float shadowFactor = 1.0;
    if (gWorldBuffer.enableShadows >= 1.0) {
      vec4 vpos = (gWorldBuffer.view * vec4(pbrInfo.WP, 1.0)).zzzz;
      int cascadeIdx = GetCascadeIndex(vpos, dynamicLightSpace.lightSpace);
      shadowFactor = GetShadowFactorCascade(gWorldBuffer.enableShadows, 
                                            pbrInfo.WP, cascadeIdx,
                                            dynamicLightSpace.lightSpace, 
                                            dynamicShadowMap,
                                            light.direction.xyz, pbrInfo.N);
    }
    //vec4 sc = GetCascadeColor(cascadeIdx);
    outColor += ambient;//sc.rgb;
    outColor += radiance * shadowFactor;
    outColor = max(outColor, ambient);
  }
  
  for (int i = 0; i < MAX_DIRECTION_LIGHTS; ++i) {
    DirectionLight light = gLightBuffer.directionLights[i];
    outColor += light.ambient.rgb * gbuffer.albedo;
    outColor += CookTorrBRDFDirectional(light, pbrInfo);
  }
  
  for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
    PointLight light = gLightBuffer.pointLights[i];
    outColor += CookTorrBRDFPoint(light, pbrInfo);
  }
  
  for (int i = 0; i < MAX_SPOT_LIGHTS; ++i) {
    SpotLight light = gLightBuffer.spotLights[i];
    outColor += CookTorrBRDFSpot(light, pbrInfo);
  }
  
  outColor = gbuffer.emissionStrength * 20.0 * gbuffer.emission + (outColor * gbuffer.ao);
  
  vec3 glow = outColor.rgb - length(gWorldBuffer.cameraPos.xyz - gbuffer.pos) * 0.2;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  vBrightColor = vec4(glow, 1.0);
  vFragColor = vec4(outColor, 1.0);
}