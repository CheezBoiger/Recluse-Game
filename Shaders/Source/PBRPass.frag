// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

in FRAG_IN {
  vec3  position;
  float pad0;
  vec3  normal;
  float pad1;
  vec2  texcoord0;
  vec2  texcoord1;
  vec4  color;
} frag_in;

#define MAX_LIGHTS            128

struct DirectionLight {
  vec4  direction;
  vec4  color;
  float intensity;
  int   enable;
  ivec2 pad;
};


struct PointLight {
  vec4    position;
  vec4    color;
  float   range;
  int     enable;
  ivec2   pad;
};


// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
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
  ivec2 screenSize;
  ivec2 pad;
} gWorldBuffer;


layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  int   hasAlbedo;
  int   hasMetallic;
  int   hasRoughness;
  int   hasNormal;
  int   hasEmissive;
  int   hasAO;
  int   hasBones; 
  int   pad;
} objBuffer;


layout (set = 1, binding = 2) uniform sampler2D albedo;
layout (set = 1, binding = 3) uniform sampler2D metallic;
layout (set = 1, binding = 4) uniform sampler2D roughness;
layout (set = 1, binding = 5) uniform sampler2D normal;
layout (set = 1, binding = 6) uniform sampler2D ao;
layout (set = 1, binding = 7) uniform sampler2D emissive;


layout (set = 2, binding = 0) uniform LightBuffer {
  DirectionLight  primaryLight;
  PointLight      pointLights[MAX_LIGHTS];
} gLightBuffer;

layout (set = 2, binding = 1) uniform sampler2D globalShadow;


// TODO(): Need to addin gridLights buffer, for light culling, too.

layout (location = 0) out vec4 finalColor;
layout (location = 1) out vec4 normalColor;


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


// Schlick-Smith GGX for Geometric shadowing.
float GSchlickSmithGGX(float NoL, float NoV, float roughness)
{
  float remap = roughness + 1.0;
  float k = (remap * remap) / 8.0;
  float GL = NoL / (NoL * (1.0 - k) + k);
  float GV = NoV / (NoV * (1.0 - k) + k);

  return (GL * GV);
}


// Schlick Approximation of our Fresnel Term
vec3 FSchlick(float cosTheta, vec3 F0, float roughness)
{
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


// Lambert Diffuse for our diffuse partial of our equation.
vec3 LambertDiffuse(vec3 kD, vec3 albedoFrag)
{
  return kD * albedoFrag / PI;
}


// TODO():
vec3 CookTorrBRDFPoint(PointLight light, vec3 albedoFrag, vec3 V, vec3 N, float roughness, float metallic)
{
  vec3 L = light.position.xyz - frag_in.position;
  vec3 nV = normalize(V);
  vec3 nL = normalize(L);
  vec3 nN = normalize(N);
  
  vec3 H = normalize(nV + nL);
  
  float dotNL = clamp(dot(nN, nL), 0.0, 1.0);
  float dotNV = clamp(dot(nN, nV), 0.0, 1.0);
  float dotLH = clamp(dot(nL, H), 0.0, 1.0);
  float dotNH = clamp(dot(nN, H), 0.0, 1.0);
  
  vec3 color = vec3(0.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, albedoFrag, metallic);
  
  float distance = length(L);
  float attenuation = light.range / ((distance * distance) + 1.0);
  vec3 radiance = light.color.xyz * attenuation;
  
  if (dotNL > 0.0) {
    float D = DGGX(dotNH, roughness);
    float G = GSchlickSmithGGX(dotNL, dotNV, roughness);
    
    vec3 F = FSchlick(dotNV, F0, roughness);
    vec3 brdf = D * F * G / (4 * dotNL * dotNV);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    color += (LambertDiffuse(kD, albedoFrag) + brdf) * radiance * dotNL;
  }
  
  return color;
}


// TODO():
vec3 CookTorrBRDFDirectional(DirectionLight light, vec3 albedoFrag, vec3 V, vec3 N, float roughness, float metallic)
{
  vec3 L = -(light.direction.xyz);
  vec3 nV = normalize(V);
  vec3 nL = normalize(L);
  vec3 nN = normalize(N);
  
  vec3 H = normalize(nV + nL);
  
  float dotNL = clamp(dot(nN, nL), 0.0, 1.0);
  float dotNV = clamp(dot(nN, nV), 0.0, 1.0);
  float dotLH = clamp(dot(nL, H), 0.0, 1.0);
  float dotNH = clamp(dot(nN, H), 0.0, 1.0);
  
  vec3 color = vec3(0.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, albedoFrag, metallic);
  
  float distance = length(L);
  vec3 radiance = light.color.xyz * light.intensity;
  
  if (dotNL > 0.0) {
    float D = DGGX(dotNH, roughness);
    float G = GSchlickSmithGGX(dotNL, dotNV, roughness);
    
    vec3 F = FSchlick(dotNV, F0, roughness);
    vec3 brdf = D * F * G / ((4 * dotNL * dotNV) + 0.001);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    color += (LambertDiffuse(kD, albedoFrag) + brdf) * radiance * dotNL;
  }
  
  return color;
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
  vec3 tNormal = texture(normal, TexCoord).rgb * 2.0 - 1.0;
  mat3 TBN = BiTangentFrame(N, -V, TexCoord);
  return normalize(TBN * tNormal);
}


void main()
{
  vec3 fragAlbedo = vec3(0.0);
  vec3 fragNormal = vec3(0.0);
  vec4 fragEmissive = vec4(0.0);
  
  float fragMetallic = 0.0;
  float fragRoughness = 0.0;
  float fragAO = 0.0;
  
  if (objBuffer.hasAlbedo >= 1) {
    fragAlbedo = pow(texture(albedo, frag_in.texcoord0).rgb, vec3(2.2));
  } else {
    fragAlbedo = frag_in.color.rgb;
  }
    
  if (objBuffer.hasMetallic >= 1) {
    fragMetallic = texture(metallic, frag_in.texcoord0).r;
  }
  
  if (objBuffer.hasRoughness >= 1) {
    fragRoughness = texture(roughness, frag_in.texcoord0).r;
  }
  
  if (objBuffer.hasNormal >= 1) {
    fragNormal = GetNormal(frag_in.normal, frag_in.position, frag_in.texcoord0);
  } else {
    fragNormal = frag_in.normal;
  }
  
  normalColor = vec4(fragNormal, 1.0);
  
  if (objBuffer.hasEmissive >= 1) {
    fragEmissive = texture(emissive, frag_in.texcoord0);
  } 
  
  if (objBuffer.hasAO >= 1) {
    fragAO = texture(ao, frag_in.texcoord0).r;
  }
    
  vec3 V = normalize(gWorldBuffer.cameraPos.xyz - frag_in.position);
  vec3 N = normalize(fragNormal);

  // Brute force lights for now.
  vec3 outColor = vec3(0.0);

  if (gLightBuffer.primaryLight.enable > 0) {
    DirectionLight light = gLightBuffer.primaryLight;
    outColor += CookTorrBRDFDirectional(light, fragAlbedo, V, N, fragRoughness, fragMetallic);   
  }
  
  for (int i = 0; i < MAX_LIGHTS; ++i) {
    PointLight light = gLightBuffer.pointLights[i];
    if (light.enable <= 0) { continue; }
    outColor += CookTorrBRDFPoint(light, fragAlbedo, V, N, fragRoughness, fragMetallic);
    
  }

  // We might wanna set a debug param here...
  finalColor = vec4(outColor, 1.0);
}