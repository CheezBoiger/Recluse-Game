// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"
#include "Common/Shadowing.glsl"
#include "Common/LightingPBR.glsl"
#include "Common/GBufferDefines.glsl"


layout (location = 0) out vec4 vFragColor;

#if !defined(ENABLE_WATER_RENDERING)
layout (location = 1) out vec4 vBrightColor;
layout (location = 2) out vec4 rt0;
layout (location = 3) out vec4 rt1;
layout (location = 4) out vec4 rt2;
layout (location = 5) out vec4 rt3;
#endif

in FRAG_IN {
  vec3 position;
  float lodBias;
  vec3 normal;
  float pad1;
  vec2 texcoord0;
  vec2 texcoord1;
  vec4 vpos;
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


layout (set = 3, binding = 0) uniform Lights {
  LightBuffer lights;
} gLightBuffer;

/////////////////////////////////////////////////////////////////////////////
// Note: Change light space to hold both Static and dynamic! Make shadow map into sampler2DArray instead!
#if defined SHADOWMAP_ARRAY
layout (set = 4, binding = 0) uniform PrimaryLightSpace {
  LightSpace staticLightSpace;
  LightSpace dynamicLightSpace;
} primaryLightSpace;

// cascading map for dynamic shadows.
layout (set = 4, binding = 1) uniform sampler2DArray cascadingShadowMapArrayD;
// Shadow map cache used by objects that don't require shadow updating.
layout (set = 4, binding = 2) uniform sampler2D shadowMapS;

#else

layout (set = 4, binding = 0) uniform sampler2D shadowMask;

/*
layout (set = 4, binding = 0) uniform DynamicLightSpace {
  LightSpaceCascade lightSpace;
} dynamicLightSpace;

layout (set = 4, binding = 1) uniform sampler2DArray dynamicShadowMap;
*/
/*
layout (set = 5, binding = 0) uniform StaticLightSpace {
  LightSpace lightSpace;
} staticLightSpace;

layout (set = 5, binding = 1) uniform sampler2D staticShadowMap;
*/

#endif

/////////////////////////////////////////////////////////////////////////////

layout (set = 5, binding = 0) buffer GlobalMapInfo {
  DiffuseSH sh;
} globalMapInfo;
layout (set = 5, binding = 1) uniform samplerCube specMap;
layout (set = 5, binding = 2) uniform sampler2D brdfLut;
#if defined(LOCAL_REFLECTIONS)
layout (set = 5, binding = 3) buffer LocalMapInfo {
  vec4      positions[32];
  vec4      minAABB[32];
  vec4      maxAABB[32];
  DiffuseSH shs[32];
} localMapInfo;
layout (set = 5, binding = 4) uniform samplerCubeArray specMaps;   // Current set enviroment map (radiance).
layout (set = 5, binding = 5) uniform sampler2DArray brdfLuts;    // BRDF lookup tables corresponding to each env map.
#endif

#if defined(ENABLE_DEBUG)
layout (push_constant) uniform Debug {
  ivec4  display;
} debug_info;
#endif

void main()
{
  vec2 offsetUV0 = matBuffer.material.offsetUV.xy;
  vec2 uv0 = frag_in.texcoord0 + offsetUV0;
  vec2 screen = gl_FragCoord.xy / vec2(gWorldBuffer.global.screenSize.xy);
  
  vec4 baseColor = matBuffer.material.color.rgba;
  vec3 fragAlbedo = baseColor.rgb;
  float transparency = matBuffer.material.opaque * baseColor.a;
  vec3 fragNormal = frag_in.normal;
  vec3 fragEmissive = vec3(0.0);

  float fragMetallic  = matBuffer.material.metal;
  float fragRoughness = matBuffer.material.rough;
  float fragAO = 1.0;  // still WIP
  
  if (matBuffer.material.hasAlbedo >= 1) {
    vec4 alb = texture(albedo, uv0, objBuffer.obj.lod);
    fragAlbedo = pow(alb.rgb, vec3(2.2));
    transparency *= alb.w;
  }
  
  if (transparency < 0.05) { discard; }
  
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
  
  vec4 vpos = frag_in.vpos;
  vec3 N = normalize(fragNormal);
  vec3 V = normalize(gWorldBuffer.global.cameraPos.xyz - frag_in.position);
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, fragAlbedo, fragMetallic);
  float NoV = clamp(abs(dot(N, V)), 0.001, 1.0);
  vec3 R = -normalize(V - 2.0 * dot(N, V) * N);
  
  PBRInfo pbrInfo;
  pbrInfo.WP = frag_in.position;
  pbrInfo.albedo = fragAlbedo;
  pbrInfo.F0 = F0;
  pbrInfo.NoV = NoV;
  pbrInfo.N = N;
  pbrInfo.V = V;
  pbrInfo.roughness = fragRoughness;
  pbrInfo.metallic = fragMetallic;
  
#if !defined(ENABLE_DEBUG)
  // Brute force lights for now.
  // TODO(): Map light probes in the future, to produce environment ambient instead.
  vec3 outColor = GetIBLContribution(pbrInfo, R, brdfLut, globalMapInfo.sh, specMap);

  if (gLightBuffer.lights.primaryLight.enable > 0) {
    DirectionLight light = gLightBuffer.lights.primaryLight;
    vec3 ambient = light.ambient.rgb * fragAlbedo;
    outColor += ambient;
    vec3 radiance = CookTorrBRDFDirectional(light, pbrInfo); 
    float shadowFactor = 1.0;
    if (gWorldBuffer.global.enableShadows >= 1.0) {
      //int cascadeIdx = GetCascadeIndex(vpos, dynamicLightSpace.lightSpace.split);
      //shadowFactor = GetShadowFactorCascade(gWorldBuffer.global.enableShadows, pbrInfo.WP, cascadeIdx,
      //                                      dynamicLightSpace.lightSpace, dynamicShadowMap);
      shadowFactor = texture(shadowMask, screen).r;
    }
    radiance *= shadowFactor;
    outColor += radiance;
    outColor = max(outColor, ambient);
  }
  
  for (int i = 0; i < MAX_DIRECTION_LIGHTS; ++i) {
    DirectionLight light = gLightBuffer.lights.directionLights[i];
    outColor += light.ambient.rgb * fragAlbedo;
    outColor += CookTorrBRDFDirectional(light, pbrInfo);
  }
  
  for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
    PointLight light = gLightBuffer.lights.pointLights[i];
    outColor += CookTorrBRDFPoint(light, pbrInfo);
  }
  
  for (int i = 0; i < MAX_SPOT_LIGHTS; ++i) {
    SpotLight light = gLightBuffer.lights.spotLights[i];
    outColor += CookTorrBRDFSpot(light, pbrInfo);
  }
  
  outColor = matBuffer.material.emissive * 20.0 * fragEmissive + (outColor * fragAO);
    
#else

#define DEBUG_ALBEDO (1<<0)
#define DEBUG_NORMAL (1<<1)
#define DEBUG_ROUGH  (1<<2)
#define DEBUG_METAL  (1<<3)
#define DEBUG_EMISSIVE (1<<4)
#define DEBUG_IBL (1<<7)
  vec3 outColor = vec3(0.0);
  int v = debug_info.display.x;
  if ((v & DEBUG_ALBEDO) == DEBUG_ALBEDO) {
    outColor += fragAlbedo;
  } 
  if ((v & DEBUG_NORMAL) == DEBUG_NORMAL) {
    outColor += N;
  }
  if ((v & DEBUG_ROUGH) == DEBUG_ROUGH){
    outColor.g += fragRoughness;
  }
  if ((v & DEBUG_METAL) == DEBUG_METAL) {
    outColor.b += fragMetallic;
  }
  if ((v & DEBUG_EMISSIVE) == DEBUG_EMISSIVE) {
    outColor += matBuffer.material.emissive * 20.0 * fragEmissive;
  }
  if ((v & DEBUG_IBL) == DEBUG_IBL) {
    outColor += GetIBLContribution(pbrInfo, R, brdfLut, globalMapInfo.sh, specMap);
  }
#endif

  vFragColor = vec4(outColor, transparency);
  
#if !defined(ENABLE_WATER_RENDERING)
  vec3 glow = outColor.rgb - length(V) * 0.2;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(50.0));
  vBrightColor = vec4(glow, 1.0);
  
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
  
  WriteGBuffer(gbuffer, rt0, rt1, rt2, rt3);
#endif
}


