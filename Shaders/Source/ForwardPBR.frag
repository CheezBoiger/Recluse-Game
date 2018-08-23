// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 vFragColor;
layout (location = 1) out vec4 vBrightColor;

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


// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  invView;
  mat4  invProj;
  mat4  viewProj;
  mat4  invViewProj;
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
  vec4  vAirColor;
  float fEngineTime; // total current time of the engine. 
  float fDeltaTime; // elapsed time between frames.
  float gamma;
  float exposure;
  float fRayleigh;
  float fMie;
  float fMieDist;
  float fScatterStrength;
  float fRayleighStength;
  float fMieStength;
  float fIntensity;
  int   bloomEnabled;
  int   enableShadows;
  int   enableAA;
  ivec2 pad;
} gWorldBuffer;


layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  float lod;          // Level of Detail
  int   hasBones; 
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

layout (set = 3, binding = 1) uniform sampler2D globalShadow;

layout (set = 4, binding = 0) uniform LightSpace {
  mat4 viewProj;
  vec4 near;
  vec4 lightSz;       // world light size / frustum width.
  vec4 shadowTechnique; // 0 for pcf, 1 for pcss
} lightSpace;


layout (set = 5, binding = 0) uniform samplerCube diffMap;
layout (set = 5, binding = 1) uniform samplerCube specMap;
layout (set = 5, binding = 2) uniform sampler2D brdfLut;
#if defined(LOCAL_REFLECTIONS)
layout (set = 5, binding = 3) uniform samplerCubeArray diffMaps;   // Current set irradiance map.
layout (set = 5, binding = 4) uniform samplerCubeArray specMaps;   // Current set enviroment map (radiance).
layout (set = 5, binding = 5) uniform sampler2DArray brdfLuts;    // BRDF lookup tables corresponding to each env map.
#endif

////////////////////////////////////////////////////////////////////////////////
// Shadowing.

#define SHADOW_FACTOR 0.0
#define SHADOW_BIAS   0.0000000000
#define BLOCKER_SEARCH_NUM_SAMPLES 16
#define PCF_NUM_SAMPLES 16
#define NEAR_PLANE 0.1
#define LIGHT_WORLD_SIZE 10.0
#define LIGHT_FRUSTUM_WIDTH 40.0

#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)

vec2 poissonDisk[16] = {
  vec2( -0.94201624,  -0.39906216),
  vec2(  0.94558609,  -0.76890725),
  vec2( -0.094184101, -0.92938870),
  vec2(  0.34495938,   0.29387760),
  vec2( -0.91588581,   0.45771432),
  vec2( -0.81544232,  -0.87912464),
  vec2( -0.38277543,   0.27676845),
  vec2(  0.97484398,   0.75648379),
  vec2(  0.44323325,  -0.97511554),
  vec2(  0.53742981,  -0.47373420),
  vec2( -0.26496911,  -0.41893023),
  vec2(  0.79197514,   0.19090188),
  vec2( -0.24188840,   0.99706507),
  vec2( -0.81409955,   0.91437590),
  vec2(  0.19984126,   0.78641367),
  vec2(  0.14383161,  -0.14100790)
};

float textureProj(vec4 P, vec2 offset)
{
  float shadow = 1.0;
  vec4 shadowCoord = P / P.w;
  shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
  
  if (shadowCoord.z <= 1.0) {
    float dist = texture(globalShadow, vec2(shadowCoord.st + offset)).r;
    if (dist < shadowCoord.z - SHADOW_BIAS) {
      shadow = SHADOW_FACTOR;
    }
  }
  return shadow;
}


float FilterPCF(vec4 sc)
{
  ivec2 texDim = textureSize(globalShadow, 0);
  float scale = 0.5;
  float dx = scale * 1.0 / float(texDim.x);
  float dy = scale * 1.0 / float(texDim.y);

  float shadowFactor = 0.0;
  float count = 0.0;
  float range = 3.5;
	
  for (float x = -range; x <= range; x++) {
    for (float y = -range; y <= range; y++) {
      shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
      count += 1.0;
    }
  }
  return shadowFactor / count;
}


float PenumbraSize(float zReceiver, float zBlocker)
{
  return (zReceiver - zBlocker) / zBlocker;
}


void FindBlocker(inout float avgBlockerDepth, inout float numBlockers, vec2 uv, float zReceiver)
{
  float lightSz = lightSpace.lightSz.x;
  float nearPlane = lightSpace.near.x;
  float searchWidth = lightSz * (zReceiver - nearPlane) / (zReceiver * 0.5 + 0.5);
  
  float blockerSum = 0;
  numBlockers = 0;
  
  for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {
    float shadowMapDepth = texture(globalShadow, uv + poissonDisk[i] * searchWidth).r;
    if (shadowMapDepth < zReceiver) {
      blockerSum += shadowMapDepth;
      numBlockers++;
    }
  }
  avgBlockerDepth = blockerSum / numBlockers;
}


float PCF_Filter(vec2 uv, float zReceiver, float filterRadiusUV)
{
  float sum = 0.0;
  for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
    float shadow = 1.0;
    if (zReceiver <= 1.0) {
      vec2 offset = poissonDisk[i] * filterRadiusUV;
      float factor = texture(globalShadow, uv + offset).r;
      if (factor < zReceiver) {
        shadow = SHADOW_FACTOR;
      }
    }
    sum += shadow;
  }
  return sum / PCF_NUM_SAMPLES;
}


float PCSS(vec4 sc)
{
  vec4 coords = sc / sc.w;
  vec2 uv = coords.st * 0.5 + 0.5;
  float zReceiver = coords.p;
  
  float avgBlockerDepth = 0;
  float numBlockers = 0;
  FindBlocker(avgBlockerDepth, numBlockers, uv, zReceiver);
  
  if (numBlockers < 1) {
    return 1.0;
  }
  
  float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
  float lightSz = lightSpace.lightSz.x;
  float near = lightSpace.near.x;
  float filterRadiusUV = penumbraRatio * lightSz * near / coords.p;
  
  return PCF_Filter(uv, zReceiver, filterRadiusUV);
}


////////////////////////////////////////////////////////////////////
// Light BRDFs
// Lighting model goes as: Cook-Torrance
//
// f = fEmissive + fDiffuse + fSpecular
//
////////////////////////////////////////////////////////////////////

const float PI = 3.14159265359;


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
    
    if (gWorldBuffer.enableShadows >= 1) {
      vec4 shadowClip = lightSpace.viewProj * vec4(vPosition, 1.0);
      float shadowFactor = ((lightSpace.shadowTechnique.x < 1) ? FilterPCF(shadowClip) : PCSS(shadowClip));
      color *= shadowFactor;
    }

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
  
  // Brute force lights for now.
  // TODO(): Map light probes in the future, to produce environment ambient instead.
  vec3 outColor = vec3(0.0);

  if (gLightBuffer.primaryLight.enable > 0) {
    DirectionLight light = gLightBuffer.primaryLight;
    vec3 ambient = light.ambient.rgb * fragAlbedo;
    outColor += ambient;
    outColor += CookTorrBRDFPrimary(light, frag_in.position, fragAlbedo, V, N, fragRoughness, fragMetallic); 
    outColor = max(outColor, ambient);
  }
  
  for (int i = 0; i < MAX_DIRECTION_LIGHTS; ++i) {
    DirectionLight light = gLightBuffer.directionLights[i];
    if (light.enable <= 0) { continue; }
    outColor += light.ambient.rgb * fragAlbedo;
    outColor += CookTorrBRDFDirectional(light, fragAlbedo, V, N, fragRoughness, fragMetallic);
  }
  
  for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
    PointLight light = gLightBuffer.pointLights[i];
    if (light.enable <= 0) { continue; }
    outColor += CookTorrBRDFPoint(light, frag_in.position, fragAlbedo, V, N, fragRoughness, fragMetallic);
    
  }
  
    outColor = matBuffer.emissive * 20.0 * fragEmissive + (outColor * fragAO);
  vFragColor = vec4(outColor, transparency);
  
  vec3 glow = outColor.rgb - length(V) * 0.2;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  vBrightColor = vec4(glow, 1.0);
}


