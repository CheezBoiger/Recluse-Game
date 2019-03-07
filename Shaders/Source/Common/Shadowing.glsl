// Copyright (c) 2018 Recluse Project. All rights reserved.
#ifndef SHADOWING_H
#define SHADOWING_H

#include "LightingPBR.glsl"

#define BLOCKER_SEARCH_NUM_SAMPLES 36
#define PCF_NUM_SAMPLES 64
#define NEAR_PLANE 0.1
#define LIGHT_WORLD_SIZE 10.0
#define LIGHT_FRUSTUM_WIDTH 40.0

#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)
#define MAX_SPOTLIGHT_SHADOWMAPS 4
#define MAX_CASCADING_SHADOWMAP_COUNT 4

struct LightSpace {
  mat4 viewProj;
  vec4 near;
  vec4 lightSz;
  vec4 shadowTechnique;
};


struct LightSpaceCascade {
  mat4 viewProj[MAX_CASCADING_SHADOWMAP_COUNT];
  vec4 split;
  vec4 near;
  vec4 lightSz;
  vec4 shadowTechnique;
};

struct SpotLightSpace {
  mat4 viewProjs[MAX_SPOTLIGHT_SHADOWMAPS]; // Only 4 spotlights at a time.
};


vec4 GetCascadeColor(int idx)
{
  vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
  if (idx == 0) {
    color = vec4(1.0, 0.0, 0.0, 1.0);
  } else if (idx == 1) {
    color = vec4(0.0, 1.0, 0.0, 1.0);
  } else if (idx == 2) {
    color = vec4(0.0, 0.0, 1.0, 0.0);
  } else if (idx == 3) {
    color = vec4(1.0, 0.0, 1.0, 1.0);
  }
  return color;
}


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







////////////////////////////////////////////////////////////////////////////////
// Shadowing.

#define SHADOW_FACTOR 0.0
#define SHADOW_BIAS   0.0000000000

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


float FilterPCF(in sampler2D shadowMap, vec4 sc, vec3 lightPos, vec3 normal, vec3 fragPos)
{
#if 0
  ivec2 texDim = textureSize(shadowMap, 0);
  float scale = 0.5;
  float dx = scale * 1.0 / float(texDim.x);
  float dy = scale * 1.0 / float(texDim.y);

  float shadowFactor = 0.0;
  float count = 0.0;
  float range = 3.5;
	
  for (float x = -range; x <= range; x += 1.0) {
    for (float y = -range; y <= range; y += 1.0) {
      shadowFactor += textureProj(shadowMap, sc, vec2(dx*x, dy*y));
      count += 1.0;
    }
  }
  return shadowFactor / count;
#endif
  vec3 projC = sc.xyz / sc.w;
  projC.st = projC.st * 0.5 + 0.5;
  float currDepth = projC.z;
  
  float shadow = 0.0;
  float range = 3.0;
  vec2 texelSz = (1.0 / textureSize(shadowMap, 0)) * 0.5;
  
  normal = normalize(normal);
  vec3 lightDir = -lightPos;
  float bias = min(-0.000001 * dot(normal, lightDir), 0.0);
  float count = 0.0;
  if (currDepth <= 1.0) {
    for (float x = -range; x <= range; x += 1.0) {
      for (float y = -range; y <= range; y += 1.0) {
        float pcfDepth = texture(shadowMap, projC.xy + vec2(x, y) * texelSz).r;
        shadow += ((currDepth - bias) > pcfDepth) ? 0.0 : 1.0;
        count += 1.0;
      }
    }
  }
  shadow /= count;
  return shadow;
}


float FilterPCFCascade(in sampler2DArray shadowMap, vec4 sc, vec3 lightPos, vec3 normal, vec3 fragPos, int layer)
{
  vec3 projC = sc.xyz / sc.w;
  projC.st = projC.st * 0.5 + 0.5;
  float currDepth = projC.z;
  
  float shadow = 0.0;
  float range = 1.0;
  vec2 texelSz = (1.0 / textureSize(shadowMap, 0).xy) * 0.5;
  
  normal = normalize(normal);
  vec3 lightDir = -lightPos;
  float bias = 0.0; //min(-0.000001 * dot(normal, lightDir), 0.0);
  float count = 0.0;
  if (currDepth <= 1.0) {
    for (float x = -range; x <= range; x += 1.0) {
      for (float y = -range; y <= range; y += 1.0) {
        float pcfDepth = texture(shadowMap, vec3(projC.xy + vec2(x, y) * texelSz, layer)).r;
        shadow += ((currDepth - bias) > pcfDepth) ? 0.0 : 1.0;
        count += 1.0;
      }
    }
  }
  shadow /= count;
  return shadow;
}


float PenumbraSize(float zReceiver, float zBlocker)
{
  return (zReceiver - zBlocker) / zBlocker;
}


void FindBlocker(in sampler2D shadowMap, 
                  in LightSpace lightSpace, 
                  inout float avgBlockerDepth, 
                  inout float numBlockers, 
                  vec2 uv, 
                  float zReceiver)
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


void FindBlockerCascade(in sampler2DArray shadowMap, 
                  int cascadeIdx,
                  in LightSpaceCascade lightSpace, 
                  inout float avgBlockerDepth, 
                  inout float numBlockers, 
                  vec2 uv, 
                  float zReceiver)
{
  float lightSz = lightSpace.lightSz.x;
  float nearPlane = lightSpace.near.x;
  float searchWidth = lightSz * (zReceiver - nearPlane) / (zReceiver * 0.5 + 0.5);
  
  float blockerSum = 0;
  numBlockers = 0;
  
  for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {
    vec2 suv = uv + poissonDisk[i] * searchWidth;
    float shadowMapDepth = texture(shadowMap, vec3(suv, cascadeIdx)).r;
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


float PCF_FilterCascade(in sampler2DArray shadowMap, int cascadeIdx, vec2 uv, float zReceiver, float filterRadiusUV)
{
  float sum = 0.0;
  for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
    float shadow = 1.0;
    if (zReceiver <= 1.0) {
      vec2 offset = poissonDisk[i] * filterRadiusUV;
      float factor = texture(shadowMap, vec3(uv + offset, cascadeIdx)).r;
      if (factor < zReceiver) {
        shadow = SHADOW_FACTOR;
      }
    }
    sum += shadow;
  }
  return sum / PCF_NUM_SAMPLES;
}


float PCSS(in sampler2D shadowMap, 
            in LightSpace lightSpace, 
            vec4 sc)
{
  vec4 coords = sc / sc.w;
  vec2 uv = coords.st * 0.5 + 0.5;
  float zReceiver = coords.p;
  
  float avgBlockerDepth = 0;
  float numBlockers = 0;
  FindBlocker(shadowMap, lightSpace, avgBlockerDepth, numBlockers, uv, zReceiver);
  
  if (numBlockers < 1) {
    return 1.0;
  }
  
  float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
  float lightSz = lightSpace.lightSz.x;
  float near = lightSpace.near.x;
  float filterRadiusUV = penumbraRatio * lightSz * near / coords.p;
  
  return PCF_Filter(shadowMap, uv, zReceiver, filterRadiusUV);
}


float PCSSCascade(in sampler2DArray shadowMap, 
                  in LightSpaceCascade lightSpace,
                  vec4 sc,
                  int cascadeIdx)
{
  vec4 coords = sc / sc.w;
  vec2 uv = coords.st * 0.5 + 0.5;
  float zReceiver = coords.p;
  
  float avgBlockerDepth = 0;
  float numBlockers = 0;
  FindBlockerCascade(shadowMap, cascadeIdx, lightSpace, avgBlockerDepth, numBlockers, uv, zReceiver);
  
  if (numBlockers < 1) {
    return 1.0;
  }
  
  float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
  float lightSz = lightSpace.lightSz.x;
  float near = lightSpace.near.x;
  float filterRadiusUV = penumbraRatio * lightSz * near / coords.p;
  
  return PCF_FilterCascade(shadowMap, cascadeIdx, uv, zReceiver, filterRadiusUV);
}


////////////////////////////////////////////////////////////////////////////////
float GetShadowFactor(int enableShadows, vec3 wp, in LightSpace staticLS, 
  in sampler2D staticSM, in LightSpace dynamicLS, in sampler2D dynamicSM, vec3 lightPos, vec3 normal)
{
    vec4 staticShadowClip = staticLS.viewProj * vec4(wp, 1.0);
    float staticShadowFactor = ((staticLS.shadowTechnique.x < 1) ? FilterPCF(staticSM, staticShadowClip, lightPos, normal, wp) : PCSS(staticSM, staticLS, staticShadowClip));
    float shadowFactor = staticShadowFactor;
    if (enableShadows >= 1) {
      vec4 shadowClip = dynamicLS.viewProj * vec4(wp, 1.0);
      float dynamicShadowFactor = ((dynamicLS.shadowTechnique.x < 1) ? FilterPCF(dynamicSM, shadowClip, lightPos, normal, wp) : PCSS(dynamicSM, dynamicLS, shadowClip));
      shadowFactor = min(dynamicShadowFactor, staticShadowFactor);
    }

    return shadowFactor;
}


int GetCascadeIndex(vec4 vpos, in LightSpaceCascade dynamicLS)
{
  
  vec4 fComparison = vec4(greaterThan(vpos, dynamicLS.split));
  vec4 cc = vec4( MAX_CASCADING_SHADOWMAP_COUNT > 0,
                  MAX_CASCADING_SHADOWMAP_COUNT > 1,
                  MAX_CASCADING_SHADOWMAP_COUNT > 2,
                  MAX_CASCADING_SHADOWMAP_COUNT > 3);
  float fIndex = dot( cc, fComparison );
  fIndex = min(fIndex, MAX_CASCADING_SHADOWMAP_COUNT);
  int cascadeIdx = int(fIndex);
  return cascadeIdx;
}


float GetShadowFactorCascade(int enableShadows, vec3 wp, int cascadeIdx, 
  in LightSpaceCascade dynamicLS, in sampler2DArray dynamicSM, vec3 lightPos, vec3 normal)
{   
    //vec4 staticShadowClip = staticLS.viewProj * vec4(wp, 1.0);
    //float staticShadowFactor = ((staticLS.shadowTechnique.x < 1) ? FilterPCF(staticSM, staticShadowClip, lightPos, normal, wp) : PCSS(staticSM, staticLS, staticShadowClip));
    float shadowFactor = 1.0;//staticShadowFactor;
    vec4 shadowClip = dynamicLS.viewProj[cascadeIdx] * vec4(wp, 1.0);
    float dynamicShadowFactor = (dynamicLS.shadowTechnique.x < 1) ? FilterPCFCascade(dynamicSM, shadowClip, lightPos, normal, wp, cascadeIdx) :
                                PCSSCascade(dynamicSM, dynamicLS, shadowClip, cascadeIdx);
    shadowFactor = dynamicShadowFactor;
    
    return shadowFactor;
}

#endif // SHADOWING_H