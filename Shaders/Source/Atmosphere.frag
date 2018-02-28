// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


in FRAG_IN {
  vec2 uv;
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


// Nitrogen absorption profile. This is used to determine color of the air.
vec3 Kr = vec3(0.18867780436772762, 0.2978442963618773, 0.7616065586417131);
#define CONST_PI 3.14159


// camera view and projection.
layout (push_constant) uniform Const {
  mat4 mInvView;
  mat4 mInvProj;
} viewer;


layout (location = 0) out vec4 FragColor;

//
// Functions from Florian Boesch 
// http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/
//

vec3 GetWorldNormal()
{
  vec2 vViewport = vec2(512.0, 512.0);
  vec2 vFragCoord = gl_FragCoord.xy / vViewport;
  vFragCoord = (vFragCoord-0.5)*2.0;
  vec4 vDeviceNormal = vec4(vFragCoord, 0.0, 1.0);
  vec4 vEyeNormal = normalize(viewer.mInvProj * vDeviceNormal);
  vec3 vWorldNormal = normalize(viewer.mInvView * vEyeNormal).xyz;
  return vWorldNormal;
}


float RayleighPhase(float mu)
{
  return 3.0 / (16.0 * CONST_PI) * (1 + mu * mu);
}


float Phase(float alpha, float g)
{
  float a =   3.0 * (1.0 - g*g);
  float b = 2.0 * (2.0 + g*g);
  float c = 1 + alpha*alpha;
  float d = pow(1.0 + g*g - 2.0 * g * alpha, 1.5);
  return (a/b)*(c/d);
}


float AtmosphericDepth(vec3 position, vec3 dir)
{
  float a = dot(dir, dir);
  float b = 2.0 * dot(dir, position);
  float c = dot(position, position) - 1.0;
  float det = b * b - 4.0 * a * c;
  float detSqrt = sqrt(det);
  float q = (-b - detSqrt) * 0.5;
  float t1 = c / q;
  return t1;
}


float HorizonExtinction(vec3 position, vec3 dir, float radius)
{
  float u = dot(dir, -position);
  if (u < 0.0) return 1.0;
  
  vec3 near = position + u * dir;
  if (length(near) < radius) return 0.0;
  else {
    vec3 v2 = normalize(near) * radius - position;
    float diff = acos(dot(normalize(v2), dir));
    return smoothstep(0.0, 1.0, pow(diff * 2.0, 3.0));
  }
}


vec3 Absorb(float dist, vec3 color, float factor)
{
  return color - color * pow(Kr, vec3(factor/dist));
}


void main()
{
  vec3  vEyeDir = GetWorldNormal();
  float alpha = dot(vEyeDir, gWorldBuffer.vSun.xyz);
  
  // Rayleigh air particles factor.
  float fRayleighFactor = Phase(alpha, -0.01) * gWorldBuffer.fRayleigh;
  
  // Mie factor to determine aerosol particles.
  float fMieFactor = Phase(alpha, gWorldBuffer.fMieDist) * gWorldBuffer.fMie;
  // Spot that forms the sun.
  float spot = smoothstep(0.0, 15.0, alpha) * gWorldBuffer.vSun.w; 
  
  vec3 vRayleighCollected = vec3(0.0);
  vec3 vMieCollected = vec3(0.0);
  
  float fSurfaceHeight = 0.0;
  float fScatterStrength = gWorldBuffer.fScatterStrength;
  float fRayleighStength = gWorldBuffer.fRayleighStength;
  float fMieStength = gWorldBuffer.fMieStength;
  float fIntensity = gWorldBuffer.fIntensity;

  int iStepCount = 16;
  vec3 vEyePos = vec3(0.0, fSurfaceHeight, 0.0);
  float fEyeDepth = AtmosphericDepth(vEyePos, vEyeDir);
  float fStepLength = fEyeDepth / float(iStepCount);
  
  for (int i = 0; i < iStepCount; ++i) {
    float fSampleDist = fStepLength * float(i);
    vec3 position = vEyePos + vEyeDir * fSampleDist;
    float fSampleDepth = AtmosphericDepth(position, gWorldBuffer.vSun.xyz);
    vec3 influx = Absorb(fSampleDepth, vec3(fIntensity), fScatterStrength);
    vRayleighCollected += Absorb(fSampleDist, Kr * influx, fRayleighStength);
    vMieCollected += Absorb(fSampleDepth, influx, fMieStength);
  }
  
  vRayleighCollected = (vRayleighCollected * pow(fEyeDepth, 16.0)) / float(iStepCount);
  vMieCollected = (vMieCollected * pow(fEyeDepth, 2.0)) / float(iStepCount);

  // TODO(): This should be the final color, not mie scattering alone...
  vec3 color = vec3(fMieFactor * vMieCollected + fRayleighFactor * vRayleighCollected);

  // TODO(): Testing Mie scattering first. Debugging Rayleigh...
  FragColor = vec4(color, 1.0);
}








