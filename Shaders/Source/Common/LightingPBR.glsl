// Copyright (c) 2018 Recluse Project. All rights reserved.
////////////////////////////////////////////////////////////////////
// Light BRDFs
// Lighting model goes as: Cook-Torrance
//
// f = fEmissive + fDiffuse + fSpecular
//
////////////////////////////////////////////////////////////////////

const float PI = 3.14159265359;

#define MAX_DIRECTION_LIGHTS    4
#define MAX_SPOT_LIGHTS         64
#define MAX_POINT_LIGHTS        64

struct PBRInfo {
  vec3 albedo;
  vec3 F0;
  vec3 N;
  vec3 V;
  float roughness;
  float NoV;
};


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


struct SpotLight {
  vec4    position;
  vec4    color;
  float   range;
  float   outer;
  float   inner;
  int     enable;
};


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


vec3 GetNormal(in sampler2D normal, float lod, vec3 N, vec3 V, vec2 TexCoord)
{
  vec3 tNormal = texture(normal, TexCoord, lod).rgb * 2.0 - 1.0;
  mat3 TBN = BiTangentFrame(N, V, TexCoord);
  return normalize(TBN * tNormal);
}


vec2 EncodeNormal(vec3 n)
{
  float p = sqrt(n.z * 8.0 + 8.0);
  return vec2(n.xy / p + 0.5);
}


vec3 DecodeNormal(vec4 enc)
{
  vec2 fenc = enc.xy * 4.0 - 2.0;
  float f = dot(fenc, fenc);
  float g = sqrt(1.0 - f / 4.0);
  vec3 n;
  n.xy = fenc * g;
  n.z = 1.0 - f / 2.0;
  return n;
}