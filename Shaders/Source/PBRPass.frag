// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

in FRAG_IN {
  vec3  position;
  float lodBias;
  vec3  normal;
  float pad1;
  vec2  texcoord0;
  vec2  texcoord1;
} frag_in;

#define MAX_DIRECTION_LIGHTS    32
#define MAX_POINT_LIGHTS        128

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
  float   intensity;
  int     enable;
  int     pad;
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
  vec2  mousePos;
  ivec2 screenSize;
  float gamma;
  float exposure;
  int   bloomEnabled;
  int   pad1;
} gWorldBuffer;


layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  int   hasBones; 
} objBuffer;


layout (set = 2, binding = 0) uniform MaterialBuffer {
  vec4  color;
  float levelOfDetail;
  float transparency;
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
} matBuffer;


layout (set = 2, binding = 1) uniform sampler2D albedo;
layout (set = 2, binding = 2) uniform sampler2D metallic;
layout (set = 2, binding = 3) uniform sampler2D roughness;
layout (set = 2, binding = 4) uniform sampler2D normal;
layout (set = 2, binding = 5) uniform sampler2D ao;
layout (set = 2, binding = 6) uniform sampler2D emissive;


layout (set = 3, binding = 0) uniform LightBuffer {
  DirectionLight  primaryLight;
  DirectionLight  directionLights[MAX_DIRECTION_LIGHTS];
  PointLight      pointLights[MAX_POINT_LIGHTS];
} gLightBuffer;

layout (set = 3, binding = 1) uniform sampler2D globalShadow;


// TODO(): Need to addin gridLights buffer, for light culling, too.

layout (location = 0) out vec4 finalColor;
layout (location = 1) out vec4 normalColor;
layout (location = 2) out vec4 BrightColor;


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


vec3 BRDF(vec3 L, vec3 albedoFrag, vec3 V, vec3 N, float roughness, float metallic)
{
  vec3 nV = normalize(V);
  vec3 nL = normalize(L);
  vec3 nN = normalize(N);
  
  vec3 H = normalize(nV + nL);
  
  float dotNL = clamp(dot(nN, nL), 0.0, 1.0);
  float dotNV = clamp(dot(nN, nV), 0.0, 1.0);
  float dotNH = clamp(dot(nN, H), 0.0, 1.0);
  
  vec3 color = vec3(0.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, albedoFrag, metallic);
  
  if (dotNL > 0.0) {
    float D = DGGX(dotNH, roughness);
    float G = GSchlickSmithGGX(dotNL, dotNV, roughness);
    
    vec3 F = FSchlick(dotNH, F0);
    vec3 brdf = D * F * G / ((4 * dotNL * dotNV) + 0.001);
    if (isnan(brdf).x == true || isinf(brdf).x == true) discard;
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    color += (LambertDiffuse(kD, albedoFrag) + brdf) * dotNL;
  }
  
  return color;
}


// TODO():
vec3 CookTorrBRDFPoint(PointLight light, vec3 Albedo, vec3 V, vec3 N, float roughness, float metallic)
{
  vec3 L = light.position.xyz - frag_in.position;
  float distance = length(L);
  // Return if range is less than the distance between light and fragment.
  if (light.range < distance) { return vec3(0.0); }

  float falloff = (distance / light.range);
  float attenuation = light.intensity - (light.intensity * falloff);
  
  vec3 radiance = light.color.xyz * attenuation;
  return BRDF(L, Albedo, V, N, roughness, metallic) * radiance;
}


// TODO():
vec3 CookTorrBRDFDirectional(DirectionLight light, vec3 Albedo, vec3 V, vec3 N, float roughness, float metallic)
{
  vec3 L = -(light.direction.xyz);
  vec3 radiance = light.color.xyz * light.intensity;
  return BRDF(L, Albedo, V, N, roughness, metallic) * radiance;
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
  mat3 TBN = BiTangentFrame(N, V, TexCoord);
  return normalize(TBN * tNormal);
}


void main()
{
  vec3 fragAlbedo = vec3(0.0);
  vec3 fragNormal = vec3(0.0);
  vec3 fragEmissive = vec3(0.0);
  
  float fragMetallic = 0.01;
  float fragRoughness = 0.1;
  float fragAO = 0.0;
  
  if (matBuffer.hasAlbedo >= 1) {
    fragAlbedo = pow(texture(albedo, frag_in.texcoord0).rgb, vec3(2.2));
  } else {
    fragAlbedo = matBuffer.color.rgb;
  }
    
  if (matBuffer.hasMetallic >= 1) {
    fragMetallic = texture(metallic, frag_in.texcoord0).r;
  } else {
    fragMetallic = matBuffer.metal;
  }
  
  if (matBuffer.hasRoughness >= 1) {
    fragRoughness = texture(roughness, frag_in.texcoord0).r;
  } else {
    fragRoughness = matBuffer.rough;
  }
  
  if (matBuffer.hasNormal >= 1) {
    fragNormal = GetNormal(frag_in.normal, frag_in.position, frag_in.texcoord0);
  } else {
    fragNormal = frag_in.normal;
  }
  
  normalColor = vec4(fragNormal, 1.0);
  
  if (matBuffer.hasEmissive >= 1) {
    fragEmissive = texture(emissive, frag_in.texcoord0).rgb;
  } 
  
  if (matBuffer.hasAO >= 1) {
    fragAO = texture(ao, frag_in.texcoord0).r;
  }
    
  vec3 V = normalize(gWorldBuffer.cameraPos.xyz - frag_in.position);
  vec3 N = normalize(fragNormal);

  // Brute force lights for now.
  vec3 ambient = vec3(0.03) * fragAlbedo; 
  vec3 outColor = (fragEmissive * matBuffer.emissive) + ambient;

  if (gLightBuffer.primaryLight.enable > 0) {
    DirectionLight light = gLightBuffer.primaryLight;
    outColor += CookTorrBRDFDirectional(light, fragAlbedo, V, N, fragRoughness, fragMetallic);   
  }
  
  for (int i = 0; i < MAX_DIRECTION_LIGHTS; ++i) {
    DirectionLight light = gLightBuffer.directionLights[i];
    if (light.enable <= 0) { continue; }
    outColor += CookTorrBRDFDirectional(light, fragAlbedo, V, N, fragRoughness, fragMetallic);
  }
  
  for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
    PointLight light = gLightBuffer.pointLights[i];
    if (light.enable <= 0) { continue; }
    outColor += CookTorrBRDFPoint(light, fragAlbedo, V, N, fragRoughness, fragMetallic);
    
  }
    
  // We might wanna set a debug param here...
  float transparency = 1.0;
  if (matBuffer.isTransparent >= 1) {
    transparency = matBuffer.transparency;
  }
  finalColor = vec4(outColor, transparency);

  // TODO(): Replace with metallness and emission instead, this will force all objects to be shiney
  // at the moment.
  vec3 glow = outColor.rgb - vec3(4.0 * fragRoughness);
  glow.r = clamp(glow.r, 0.0, 1.0);
  glow.g = clamp(glow.g, 0.0, 1.0);
  glow.b = clamp(glow.b, 0.0, 1.0);
  BrightColor = vec4(glow, 1.0);
}
















