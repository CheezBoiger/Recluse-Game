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
  vec4  vSun; // Sundir.xyz and w is brightness.
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
layout (set = 2, binding = 2) uniform sampler2D metallic;
layout (set = 2, binding = 3) uniform sampler2D roughness;
layout (set = 2, binding = 4) uniform sampler2D normal;
layout (set = 2, binding = 5) uniform sampler2D ao;
layout (set = 2, binding = 6) uniform sampler2D emissive;


layout (location = 0) out vec4 rt0;
layout (location = 1) out vec4 rt1;
layout (location = 2) out vec4 rt2;
layout (location = 3) out vec4 rt3;


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


vec2 EncodeNormal(vec3 n)
{
  vec2 enc = normalize(n.xy) * (sqrt(-n.z * 0.5 + 0.5));
  enc = enc * 0.5 + 0.5;
  return enc;
}


struct GBuffer
{
  vec3 albedo;
  vec3 normal;
  vec3 pos;
  vec3 emission;
  float roughness;
  float metallic;
};


void WriteGBuffer(GBuffer gbuffer)
{
  rt0 = vec4(gbuffer.albedo, 1.0);
  rt1 = vec4(EncodeNormal(gbuffer.normal), gbuffer.roughness, gbuffer.metallic);
  rt2 = vec4(gbuffer.pos, 1.0);
  rt3 = vec4(gbuffer.emission, 1.0);
}


void main()
{
  vec3 fragAlbedo = vec3(0.0);
  vec3 fragNormal = vec3(0.0);
  vec3 fragEmissive = vec3(0.0);

  float fragMetallic = 0.01;
  float fragRoughness = 0.1;
  float fragAO = 0.0;  // still WIP
  
  if (matBuffer.hasAlbedo >= 1) {
    fragAlbedo = pow(texture(albedo, frag_in.texcoord0, objBuffer.lod).rgb, vec3(2.2));
  } else {
    fragAlbedo = matBuffer.color.rgb;
  }
    
  if (matBuffer.hasMetallic >= 1) {
    fragMetallic = texture(metallic, frag_in.texcoord0, objBuffer.lod).r;
  } else {
    fragMetallic = matBuffer.metal;
  }
  
  if (matBuffer.hasRoughness >= 1) {
    fragRoughness = texture(roughness, frag_in.texcoord0, objBuffer.lod).r;
  } else {
    fragRoughness = matBuffer.rough;
  }
  
  if (matBuffer.hasNormal >= 1) {
    fragNormal = GetNormal(frag_in.normal, frag_in.position, frag_in.texcoord0);
  } else {
    fragNormal = frag_in.normal;
  }
  
  if (matBuffer.hasAO >= 1) {
    fragAO = texture(ao, frag_in.texcoord0, objBuffer.lod).r;
  }
  
  if (matBuffer.hasEmissive >= 1) {
    fragEmissive = texture(emissive, frag_in.texcoord0, objBuffer.lod).rgb;
  }   
  
  vec3 N = normalize(fragNormal);
  
  GBuffer gbuffer;
  gbuffer.albedo = fragAlbedo;
  gbuffer.pos = frag_in.position;
  gbuffer.normal = N;
  gbuffer.emission = fragEmissive * matBuffer.emissive;
  gbuffer.metallic = fragMetallic;
  gbuffer.roughness = fragRoughness;
  
  WriteGBuffer(gbuffer);
}