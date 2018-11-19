// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"


layout (location = 0) out vec4 vFragColor;

#if !defined(ENABLE_WATER_RENDERING)
layout (location = 1) out vec4 vBrightColor;
layout (location = 2) out vec4 rt0;
layout (location = 3) out vec4 rt1;
layout (location = 4) out vec4 rt2;
layout (location = 5) out vec4 rt3;


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


void WriteGBuffer(GBuffer gbuffer)
{
  rt0 = vec4(gbuffer.albedo, gbuffer.ao);
  rt1 = vec4(gbuffer.normal * 0.5 + 0.5, gbuffer.anisoSpec.x);
  rt2 = vec4(gbuffer.emissionStrength, gbuffer.roughness, gbuffer.metallic, gbuffer.anisoSpec.y);
  rt3 = vec4(gbuffer.emission, 0.0);
}
#endif

#define MAX_DIRECTION_LIGHTS    4
#define MAX_POINT_LIGHTS        64

struct DirectionLight {
  vec4  direction;
  vec4  ambient;
  vec4  color;
  float intensity;
  int   enable;
  ivec2 pad;
};


struct PointLight {
  vec4    position;
  vec4    color;
  float   range;
  float   intensity;
  int     enable;
  int     pad;
};

in FRAG_IN {
  vec3 position;
  float lodBias;
  vec3 normal;
  float pad1;
  vec2 texcoord0;
  vec2 texcoord1;
} frag_in;


struct PBRInfo {
  vec3 albedo;
  vec3 F0;
  vec3 N;
  vec3 V;
  float roughness;
  float NoV;
};


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


layout (set = 3, binding = 0) uniform LightBuffer {
  DirectionLight  primaryLight;
  DirectionLight  directionLights[MAX_DIRECTION_LIGHTS];
  PointLight      pointLights[MAX_POINT_LIGHTS];
} gLightBuffer;

layout (set = 4, binding = 0) uniform LightSpace {
  mat4 viewProj;
  vec4 near;
  vec4 lightSz;       // world light size / frustum width.
  vec4 shadowTechnique; // 0 for pcf, 1 for pcss
} lightSpace;

layout (set = 4, binding = 1) uniform sampler2D dynamicShadowMap;

layout (set = 5, binding = 0) uniform StaticLightSpace {
  mat4 viewProj;
  vec4 near;
  vec4 lightSz;       // world light size / frustum width.
  vec4 shadowTechnique; // 0 for pcf, 1 for pcss
} staticLightSpace;

layout (set = 5, binding = 1) uniform sampler2D staticShadowMap;

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

#if defined(ENABLE_DEBUG)
layout (push_constant) uniform Debug {
  ivec4  display;
} debug_info;
#endif


////////////////////////////////////////////////////////////////////////////////
// Shadowing.

#define SHADOW_FACTOR 0.0
#define SHADOW_BIAS   0.0000000000
#define BLOCKER_SEARCH_NUM_SAMPLES 32
#define PCF_NUM_SAMPLES 64
#define NEAR_PLANE 0.1
#define LIGHT_WORLD_SIZE 10.0
#define LIGHT_FRUSTUM_WIDTH 40.0

#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)

vec2 poissonDisk[64] = {
  vec2( -0.613392,  0.617481),
  vec2(  0.170019, -0.040254),
  vec2( -0.299417,  0.791925),
  vec2(  0.645680,  0.493210),
  vec2( -0.651784,  0.717887),
  vec2(  0.421003,  0.027070),
  vec2( -0.817194, -0.271096),
  vec2( -0.705374, -0.668203),
  vec2(  0.977050, -0.108615),
  vec2(  0.063326,  0.142369),
  vec2(  0.203528,  0.214331),
  vec2( -0.667531,  0.326090),
  vec2( -0.098422, -0.295755),
  vec2( -0.885922,  0.215369),
  vec2(  0.566637,  0.605213),
  vec2(  0.039766, -0.396100),
  vec2(  0.751946,  0.453352),
  vec2(  0.078707, -0.715323),
  vec2( -0.075838, -0.529344),
  vec2(  0.724479, -0.580798),
  vec2(  0.222999, -0.215125),
  vec2( -0.467574, -0.405438),
  vec2( -0.248268, -0.814753),
  vec2(  0.354411, -0.887570),
  vec2(  0.175817,  0.382366),
  vec2(  0.487472, -0.063082),
  vec2( -0.084078,  0.898312),
  vec2(  0.488876, -0.783441),
  vec2(  0.470016,  0.217933),
  vec2( -0.696890, -0.549791),
  vec2( -0.149693,  0.605762),
  vec2(  0.034211,  0.979980),
  vec2(  0.503098, -0.308878),
  vec2( -0.016205, -0.872921),
  vec2(  0.385784, -0.393902),
  vec2( -0.146886, -0.859249),
  vec2(  0.643361,  0.164098),
  vec2(  0.634388, -0.049471),
  vec2( -0.688894,  0.007843),
  vec2(  0.464034, -0.188818),
  vec2( -0.440840,  0.137486),
  vec2(  0.364483,  0.511704),
  vec2(  0.034028,  0.325968),
  vec2(  0.099094, -0.308023),
  vec2(  0.693960, -0.366253),
  vec2(  0.678884, -0.204688),
  vec2(  0.001801,  0.780328),
  vec2(  0.145177, -0.898984),
  vec2(  0.062655, -0.611866),
  vec2(  0.315226, -0.604297),
  vec2( -0.780145,  0.486251),
  vec2( -0.371868,  0.882138),
  vec2(  0.200476,  0.494430),
  vec2( -0.494552, -0.711051),
  vec2(  0.612476,  0.705252),
  vec2( -0.578845, -0.768792),
  vec2( -0.772454, -0.090976),
  vec2(  0.504440,  0.372295),
  vec2(  0.155736,  0.065157),
  vec2(  0.391522,  0.849605),
  vec2( -0.620106, -0.328104),
  vec2(  0.789239, -0.419965),
  vec2( -0.545396,  0.538133),
  vec2( -0.178564, -0.596057),
};

float textureProj(in sampler2D shadowMap, vec4 P, vec2 offset)
{
  float shadow = 1.0;
  vec4 shadowCoord = P / P.w;
  shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
  
  if (shadowCoord.z <= 1.0) {
    vec2 shadowUV = vec2(shadowCoord.st + offset);
    float dist = texture(shadowMap, shadowUV).r;
    if (dist < shadowCoord.z - SHADOW_BIAS) {
      shadow = SHADOW_FACTOR;
    }
  }
  return shadow;
}


float FilterPCF(in sampler2D shadowMap, vec4 sc)
{
  ivec2 texDim = textureSize(shadowMap, 0);
  float scale = 0.5;
  float dx = scale * 1.0 / float(texDim.x);
  float dy = scale * 1.0 / float(texDim.y);

  float shadowFactor = 0.0;
  float count = 0.0;
  float range = 3.5;
	
  for (float x = -range; x <= range; x++) {
    for (float y = -range; y <= range; y++) {
      shadowFactor += textureProj(shadowMap, sc, vec2(dx*x, dy*y));
      count += 1.0;
    }
  }
  return shadowFactor / count;
}


float PenumbraSize(float zReceiver, float zBlocker)
{
  return (zReceiver - zBlocker) / zBlocker;
}


void FindBlocker(in sampler2D shadowMap, inout float avgBlockerDepth, inout float numBlockers, vec2 uv, float zReceiver)
{
  float lightSz = lightSpace.lightSz.x;
  float nearPlane = lightSpace.near.x;
  float searchWidth = lightSz * (zReceiver - nearPlane) / (zReceiver * 0.5 + 0.5);
  
  float blockerSum = 0;
  numBlockers = 0;
  
  for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {
    vec2 suv = uv + poissonDisk[i] * searchWidth;
    float shadowMapDepth = texture(shadowMap, suv).r;
    if (shadowMapDepth < zReceiver) {
      blockerSum += shadowMapDepth;
      numBlockers++;
    }
  }
  avgBlockerDepth = blockerSum / numBlockers;
}


float PCF_Filter(in sampler2D shadowMap, vec2 uv, float zReceiver, float filterRadiusUV)
{
  float sum = 0.0;
  for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
    float shadow = 1.0;
    if (zReceiver <= 1.0) {
      vec2 offset = poissonDisk[i] * filterRadiusUV;
      float factor = texture(shadowMap, uv + offset).r;
      if (factor < zReceiver) {
        shadow = SHADOW_FACTOR;
      }
    }
    sum += shadow;
  }
  return sum / PCF_NUM_SAMPLES;
}


float PCSS(in sampler2D shadowMap, vec4 sc)
{
  vec4 coords = sc / sc.w;
  vec2 uv = coords.st * 0.5 + 0.5;
  float zReceiver = coords.p;
  
  float avgBlockerDepth = 0;
  float numBlockers = 0;
  FindBlocker(shadowMap, avgBlockerDepth, numBlockers, uv, zReceiver);
  
  if (numBlockers < 1) {
    return 1.0;
  }
  
  float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
  float lightSz = lightSpace.lightSz.x;
  float near = lightSpace.near.x;
  float filterRadiusUV = penumbraRatio * lightSz * near / coords.p;
  
  return PCF_Filter(shadowMap, uv, zReceiver, filterRadiusUV);
}


////////////////////////////////////////////////////////////////////
// Light BRDFs
// Lighting model goes as: Cook-Torrance
//
// f = fEmissive + fDiffuse + fSpecular
//
////////////////////////////////////////////////////////////////////

const float PI = 3.14159265359;


vec4 SRGBToLINEAR(vec4 srgbIn)
{
  vec3 linOut = pow(srgbIn.xyz, vec3(2.2));
  return vec4(linOut, srgbIn.w);
}


vec3 GetIBLContribution(inout PBRInfo pbrInfo, 
  vec3 reflection,
  in sampler2D brdfLUT, 
  in samplerCube diffuseCube, 
  in samplerCube specCube)
{
  float mipCount = 9.0;
  float lod = pbrInfo.roughness * mipCount;
  vec3 brdf = SRGBToLINEAR(texture(brdfLUT, vec2(pbrInfo.NoV, 1.0 - pbrInfo.roughness))).rgb;
  vec3 diffuseLight = SRGBToLINEAR(texture(diffuseCube, pbrInfo.N)).rgb;
  
#if defined(USE_TEX_LOD)
#else
  vec3 specularLight = SRGBToLINEAR(texture(specCube, reflection)).rgb;
#endif
  vec3 diffuse = diffuseLight * pbrInfo.albedo;
  vec3 specular = specularLight * (pbrInfo.F0 * brdf.x + brdf.y);
  return specular;
}


// Trowbridge-Reitz to calculate the Roughness
float DGGX(float NoH, float roughness)
{
  float alpha = (roughness * roughness);
  float alpha2 = alpha * alpha;
  float denom = (NoH * NoH) * (alpha2 - 1.0) + 1.0;
  return alpha2 / (PI * (denom * denom));
}


float GGXSchlickApprox(float NoV, float roughness)
{
  float remap = roughness + 1.0;
  float k = (remap * remap) / 8.0;
  float num = NoV;
  float denom = (NoV * (1.0 - k) + k);

  return num / denom; 
}


// Schlick-Smith GGX for Geometric shadowing.
float GSchlickSmithGGX(float NoL, float NoV, float roughness)
{
  float ggx1 = GGXSchlickApprox(NoL, roughness);
  float ggx2 = GGXSchlickApprox(NoV, roughness);
  return ggx1 * ggx2;
}


// Schlick Approximation of our Fresnel Term
vec3 FSchlick(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


// Lambert Diffuse for our diffuse partial of our equation.
vec3 LambertDiffuse(vec3 kD, vec3 albedoFrag)
{
  return kD * albedoFrag / PI;
}


vec3 BRDF(float D, vec3 F, float G, float NoL, float NoV)
{
  vec3 brdf = D * F * G / (4 * NoL * NoV);
  return brdf;
}


// TODO():
vec3 CookTorrBRDFPoint(PointLight light, vec3 vPosition, vec3 Albedo, float roughness, float metallic, inout PBRInfo pbrInfo)
{
  vec3 L = light.position.xyz - vPosition;
  float distance = length(L);
  // Return if range is less than the distance between light and fragment.
  if (light.range < distance) { return vec3(0.0); }

  vec3 color = vec3(0.0);
  float falloff = (distance / light.range);
  float attenuation = light.intensity - (light.intensity * falloff);
  vec3 nL = normalize(L);
  
  vec3 H = normalize(pbrInfo.V + nL);
  
  float NoL = clamp(dot(pbrInfo.N, nL), 0.001, 1.0);
  float NoH = clamp(dot(pbrInfo.N, H), 0.0, 1.0);
  float VoH = clamp(dot(pbrInfo.V, H), 0.0, 1.0);
  vec3 radiance = light.color.xyz * attenuation;
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, pbrInfo.NoV, roughness);  
    vec3 F = FSchlick(VoH, pbrInfo.F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
  
    color += (LambertDiffuse(kD, Albedo) + BRDF(D, F, G, NoL, pbrInfo.NoV)) * NoL;
  }
  return color * radiance;
}


// TODO():
vec3 CookTorrBRDFDirectional(DirectionLight light, vec3 Albedo, float roughness, float metallic, inout PBRInfo pbrInfo)
{
  vec3 color = vec3(0.0);
  vec3 L = -(light.direction.xyz);
  vec3 radiance = light.color.xyz * light.intensity;
  vec3 nL = normalize(L);
  
  vec3 H = normalize(pbrInfo.V + nL);
  
  float NoL = clamp(dot(pbrInfo.N, nL), 0.001, 1.0);
  float NoV = clamp(abs(dot(pbrInfo.N, pbrInfo.V)), 0.001, 1.0);
  float NoH = clamp(dot(pbrInfo.N, H), 0.0, 1.0);
  float VoH = clamp(dot(pbrInfo.V, H), 0.0, 1.0);
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, pbrInfo.NoV, roughness);  
    vec3 F = FSchlick(VoH, pbrInfo.F0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    color += (LambertDiffuse(kD, Albedo) + BRDF(D, F, G, NoL, pbrInfo.NoV)) * NoL;
  }
  return color * radiance;
}


// TODO(): This will eventually be integrated into Directional, as we will need to 
// support more shadow maps (using a Sampler2DArray.)
vec3 CookTorrBRDFPrimary(DirectionLight light, vec3 vPosition, vec3 Albedo, float roughness, float metallic, inout PBRInfo pbrInfo)
{
  vec3 color = vec3(0.0);
  vec3 L = -(light.direction.xyz);
  vec3 radiance = light.color.xyz * light.intensity;
  vec3 nL = normalize(L);
  
  vec3 H = normalize(pbrInfo.V + nL);
  
  float NoL = clamp(dot(pbrInfo.N, nL), 0.001, 1.0);
  float NoV = clamp(abs(dot(pbrInfo.N, pbrInfo.V)), 0.001, 1.0);
  float NoH = clamp(dot(pbrInfo.N, H), 0.0, 1.0);
  float VoH = clamp(dot(pbrInfo.V, H), 0.0, 1.0);
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, pbrInfo.NoV, roughness);  
    vec3 F = FSchlick(VoH, pbrInfo.F0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    
    color += LambertDiffuse(kD, Albedo) + BRDF(D, F, G, NoL, pbrInfo.NoV);
    
    vec4 staticShadowClip = staticLightSpace.viewProj * vec4(vPosition, 1.0);
    float staticShadowFactor = ((staticLightSpace.shadowTechnique.x < 1) ? FilterPCF(staticShadowMap, staticShadowClip) : PCSS(staticShadowMap, staticShadowClip));
    float shadowFactor = staticShadowFactor;
    if (gWorldBuffer.enableShadows >= 1) {
      vec4 shadowClip = lightSpace.viewProj * vec4(vPosition, 1.0);
      float dynamicShadowFactor = ((lightSpace.shadowTechnique.x < 1) ? FilterPCF(dynamicShadowMap, shadowClip) : PCSS(dynamicShadowMap, shadowClip));
      shadowFactor = min(dynamicShadowFactor, staticShadowFactor);
    }
    
    color *= shadowFactor;
    color *= NoL;
  }
  return color * radiance;
}


////////////////////////////////////////////////////////////////////

mat3 BiTangentFrame(vec3 Normal, vec3 Position, vec2 UV)
{
  vec3 dp1 = dFdx(Position);
  vec3 dp2 = dFdy(Position);
  
  vec2 duv1 = dFdx(UV);
  vec2 duv2 = dFdy(UV);
  
  vec3 N = normalize(Normal);
  vec3 T = normalize(dp1 * duv2.t - dp2 * duv1.t);
  vec3 B = -normalize(cross(N, T));
  
  return mat3(T, B, N);
}


vec3 GetNormal(vec3 N, vec3 V, vec2 TexCoord)
{
  vec3 tNormal = texture(normal, TexCoord, objBuffer.lod).rgb * 2.0 - 1.0;
  mat3 TBN = BiTangentFrame(N, V, TexCoord);
  return normalize(TBN * tNormal);
}


void main()
{
  vec2 offsetUV0 = matBuffer.offsetUV.xy;
  vec2 uv0 = frag_in.texcoord0 + offsetUV0;
  
  vec4 baseColor = matBuffer.color.rgba;
  vec3 fragAlbedo = baseColor.rgb;
  float transparency = matBuffer.opaque * baseColor.a;
  vec3 fragNormal = frag_in.normal;
  vec3 fragEmissive = vec3(0.0);

  float fragMetallic  = matBuffer.metal;
  float fragRoughness = matBuffer.rough;
  float fragAO = 1.0;  // still WIP
  
  if (matBuffer.hasAlbedo >= 1) {
    vec4 alb = texture(albedo, uv0, objBuffer.lod);
    fragAlbedo = pow(alb.rgb, vec3(2.2));
    transparency *= alb.w;
  }
  
  if (transparency < 0.05) { discard; }
  
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
    fragNormal = GetNormal(frag_in.normal, frag_in.position, uv0);
  }
  
  if (matBuffer.hasAO >= 1) {
    fragAO = texture(ao, uv0, objBuffer.lod).r;
  }
  
  if (matBuffer.hasEmissive >= 1) {
    fragEmissive = pow(texture(emissive, uv0, objBuffer.lod).rgb, vec3(2.2));
  }   
  
  vec3 N = normalize(fragNormal);
  vec3 V = normalize(gWorldBuffer.cameraPos.xyz - frag_in.position);
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, fragAlbedo, fragMetallic);
  float NoV = clamp(abs(dot(N, V)), 0.001, 1.0);
  vec3 R = -normalize(V - 2.0 * dot(N, V) * N);
  
  PBRInfo pbrInfo;
  pbrInfo.albedo = fragAlbedo;
  pbrInfo.F0 = F0;
  pbrInfo.NoV = NoV;
  pbrInfo.N = N;
  pbrInfo.V = V;
  pbrInfo.roughness = fragRoughness;
  
#if !defined(ENABLE_DEBUG)
  // Brute force lights for now.
  // TODO(): Map light probes in the future, to produce environment ambient instead.
  vec3 outColor = GetIBLContribution(pbrInfo, R, brdfLut, specMap, specMap);

  if (gLightBuffer.primaryLight.enable > 0) {
    DirectionLight light = gLightBuffer.primaryLight;
    vec3 ambient = light.ambient.rgb * fragAlbedo;
    outColor += ambient;
    outColor += CookTorrBRDFPrimary(light, frag_in.position, fragAlbedo, fragRoughness, fragMetallic, pbrInfo); 
    outColor = max(outColor, ambient);
  }
  
  for (int i = 0; i < MAX_DIRECTION_LIGHTS; ++i) {
    DirectionLight light = gLightBuffer.directionLights[i];
    if (light.enable <= 0) { continue; }
    outColor += light.ambient.rgb * fragAlbedo;
    outColor += CookTorrBRDFDirectional(light, fragAlbedo, fragRoughness, fragMetallic, pbrInfo);
  }
  
  for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
    PointLight light = gLightBuffer.pointLights[i];
    if (light.enable <= 0) { continue; }
    outColor += CookTorrBRDFPoint(light, frag_in.position, fragAlbedo, fragRoughness, fragMetallic, pbrInfo);
  }
  
    outColor = matBuffer.emissive * 20.0 * fragEmissive + (outColor * fragAO);
    
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
    outColor += matBuffer.emissive * 20.0 * fragEmissive;
  }
  if ((v & DEBUG_IBL) == DEBUG_IBL) {
    outColor += GetIBLContribution(pbrInfo, R, brdfLut, specMap, specMap);
  }
#endif

  vFragColor = vec4(outColor, transparency);
  
#if !defined(ENABLE_WATER_RENDERING)
  vec3 glow = outColor.rgb - length(V) * 0.2;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  vBrightColor = vec4(glow, 1.0);
  
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
  
  WriteGBuffer(gbuffer);
#endif
}


