// Copyright (c) 2017 Recluse Project. All rights reserved.
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
  vec2 uv;
} frag_in;


layout (set = 0, binding = 0) uniform GlobalBuffer {
  mat4  view;
  mat4  proj;
  mat4  invView;
  mat4  invProj;
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

layout (set = 2, binding = 1) uniform sampler2D globalShadow;

layout (set = 3, binding = 0) uniform LightSpace {
  mat4 viewProj;
} lightSpace;

//layout (set = 4, binding = 0) uniform samplerCubeArray envMaps;


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
};


vec3 DecodeNormal(vec4 enc)
{
  vec4 nn = enc * vec4(2.0, 2.0, 0.0, 0.0) + vec4(-1.0, -1.0, 1.0, -1.0);
  float l = dot(nn.xyz, -nn.xyw);
  nn.z = l;
  nn.xy *= sqrt(l);
  return nn.xyz * 2.0 + vec3(0.0, 0.0, -1.0);
}


GBuffer ReadGBuffer(vec2 uv)
{
  GBuffer gbuffer;
  vec4 albedoFrag           = texture(rt0, uv);
  vec4 normalFrag           = texture(rt1, uv);
  vec4 erm                  = texture(rt2, uv);
  vec4 emissionFrag         = texture(rt3, uv);
  vec4 encodedN             = vec4(normalFrag.xy, 0.0, 0.0);
  
  gbuffer.albedo            = albedoFrag.rgb;
  gbuffer.normal            = DecodeNormal(encodedN);
  gbuffer.emission          = emissionFrag.rgb;
  gbuffer.emissionStrength  = erm.r;
  gbuffer.roughness         = erm.g;
  gbuffer.metallic          = erm.b;              // metallic
  gbuffer.ao                = albedoFrag.a;       // ao

  // 0 roughness equates to no surface to render with light. Speeds up performance.
  // TODO(): We need a stencil surface to determine what parts of the 
  // image to render with light. This is not a good way.
  if (erm.a > 0.0) { discard; }

  float z = texture(rtDepth, uv).r;
  float x = uv.x * 2.0 - 1.0;
  float y = uv.y * 2.0 - 1.0;
  vec4 vProjectedPos = vec4(x, y, z, 1.0);
  vec4 vViewPos = gWorldBuffer.invProj * vProjectedPos;
  vViewPos /= vViewPos.w;
  vec4 vWorldPos = gWorldBuffer.invView * vViewPos;
  gbuffer.pos = vWorldPos.xyz;
  
  return gbuffer;
}


////////////////////////////////////////////////////////////////////////////////
// Shadowing.

#define SHADOW_FACTOR 0.0
#define SHADOW_BIAS   0.0000001

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
  vec3 brdf = D * F * G / ((4 * NoL * NoV) + 0.001);
  if (isnan(brdf).x == true || isinf(brdf).x == true) discard;
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
  
  float NoL = clamp(dot(nN, nL), 0.0, 1.0);
  float NoV = clamp(dot(nN, nV), 0.0, 1.0);
  float NoH = clamp(dot(nN, H), 0.0, 1.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, Albedo, metallic);
  vec3 radiance = light.color.xyz * attenuation;
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, NoV, roughness);  
    vec3 F = FSchlick(NoH, F0);
    
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
  
  float NoL = clamp(dot(nN, nL), 0.0, 1.0);
  float NoV = clamp(dot(nN, nV), 0.0, 1.0);
  float NoH = clamp(dot(nN, H), 0.0, 1.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, Albedo, metallic);
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, NoV, roughness);  
    vec3 F = FSchlick(NoH, F0);
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
  
  float NoL = clamp(dot(nN, nL), 0.0, 1.0);
  float NoV = clamp(dot(nN, nV), 0.0, 1.0);
  float NoH = clamp(dot(nN, H), 0.0, 1.0);
  vec3 F0 = vec3(0.04);
  
  F0 = mix(F0, Albedo, metallic);
  
  if (NoL > 0.0) {
    float D = DGGX(NoH, roughness);
    float G = GSchlickSmithGGX(NoL, NoV, roughness);  
    vec3 F = FSchlick(NoH, F0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    
    color += LambertDiffuse(kD, Albedo) + BRDF(D, F, G, NoL, NoV);
    
    if (gWorldBuffer.enableShadows >= 1) {
      vec4 shadowClip = lightSpace.viewProj * vec4(vPosition, 1.0);
      float shadowFactor = FilterPCF(shadowClip);
      color *= shadowFactor;
    }

    color *= NoL;
  }
  return color * radiance;
}


void main()
{
  GBuffer gbuffer = ReadGBuffer(frag_in.uv);

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
  
  outColor = gbuffer.emissionStrength * gbuffer.emission + (outColor * gbuffer.ao);
  vFragColor = vec4(outColor, 1.0);
  
  vec3 glow = outColor.rgb - length(gWorldBuffer.cameraPos.xyz - gbuffer.pos) * 0.2;
  glow = max(glow, vec3(0.0));
  glow = glow * 0.02;
  glow = clamp(glow, vec3(0.0), vec3(1.0));
  vBrightColor = vec4(glow, 1.0);
}