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

#define MAX_LIGHTS            512

struct DirectionLight {
  vec4 direction;
  vec4 color;
  bool enable;
  bool pad[15];
};


struct PointLight {
  vec4  position;
  vec4  color;
  float range;
  float pad0[3];
  bool enable;
  bool pad1[15];
};


// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  viewProj;
  mat4  cameraView;
  mat4  cameraProj;
  vec4  cameraPos;
  float coffSH[9];
  float pad1[3];
} gWorldBuffer;


layout (set = 1, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat3  inverseNormalMatrix;
  float pad0[3];
  bool  hasAlbedo;
  bool  hasMetallic;
  bool  hasRoughness;
  bool  hasNormal;
  bool  hasAO;
  bool  hasBones;
  bool  pad1[10];
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


// TODO(): Need to addin gridLights buffer, for light culling, too.

layout (location = 0) out vec4 OutColor;


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
vec3 LambertDiffuse(float kD, vec3 albedoFrag)
{
  return kD * albedoFrag / PI;
}


// TODO():
vec3 CookTorrBRDF()
{
  return vec3(0.0);
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
  
  float fragMetallic = 0.0;
  float fragRoughness = 0.0;
  float fragAO = 0.0;
  
  if (objBuffer.hasAlbedo) {
    fragAlbedo = pow(texture(albedo, frag_in.texcoord0).rgb, vec3(2.2));
  } else {
    fragAlbedo = frag_in.color.rgb;
  }
    
  if (objBuffer.hasMetallic) {
    fragMetallic = texture(metallic, frag_in.texcoord0).r;
  }
  
  if (objBuffer.hasRoughness) {
    fragRoughness = texture(roughness, frag_in.texcoord0).r;
  }
  
  if (objBuffer.hasNormal) {
    fragNormal = GetNormal(frag_in.normal, frag_in.position, frag_in.texcoord0);
  } else {
    fragNormal = frag_in.normal;
  }
  
  if (objBuffer.hasAO) {
    fragAO = texture(ao, frag_in.texcoord0).r;
  }
    
  vec3 V = normalize(gWorldBuffer.cameraPos.xyz - frag_in.position);
  vec3 N = normalize(fragNormal);
  
  
  OutColor = vec4(fragAlbedo, 1.0);
}