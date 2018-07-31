// Copyright (c) 2017 Recluse Project. All rights reserved.

// HDR shader.
// Reference: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/7.bloom/7.bloom_final.fs
//
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 fragColor;

in FRAG_IN {
  vec2 position;
  vec2 uv;
} frag_in;


layout (set = 0, binding = 0) uniform sampler2D sceneSurface;
layout (set = 0, binding = 1) uniform sampler2D bloomSurface;


// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 2) uniform GlobalBuffer {
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


layout (set = 1, binding = 0) uniform HDRConfig {
  vec4 bAllowChromaticAberration;
  vec4 k;
  vec4 kcube;
  vec4 bInterleaveVideo;
  vec4 interleaveShakeInterval;
} hdr;


// TODO(): We might turn this into a descriptor set instead...
layout (push_constant) uniform ParamsHDR {
  float bloomStrength;
} paramConfigs;


vec3 Uncharted2Tonemap(vec3 x)
{
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

float random(vec2 c){
  return fract(sin(dot(c.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

//
// Description : Array and textureless GLSL 2D/3D/4D simplex
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}


float snoise3(vec3 v)
{
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

  // First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

  // Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

  // Permutations
  i = mod289(i);
  vec4 p = permute( permute( permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

  // Gradients: 7x7 points over a square, mapped onto an octahedron.
  // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

  //Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

  // Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                dot(p2,x2), dot(p3,x3) ) );
}


void main()
{
  vec2 uv = frag_in.uv;
  vec3 color = texture(sceneSurface, uv).rgb;
  vec3 bloom = texture(bloomSurface, uv).rgb;
  
  // TODO(): These need to be set as a descripter param instead.
  float k = -0.15;
  float kcube = 0.15;
  
  if (hdr.bAllowChromaticAberration.x >= 1.0) {
    float r2 = (uv.x - 0.5) * (uv.x - 0.5) + (uv.y - 0.5) * (uv.y - 0.5);
    float f = 0;
    if (hdr.kcube.x == 0.0) {
      f = 1.0 + r2 * hdr.k.x;
    } else {
      f = 1.0 + r2 * (hdr.k.x + hdr.kcube.x * sqrt(r2));
    }
    
    float ux = f*(uv.x-0.5)+0.5;
    float uy = f*(uv.y-0.5)+0.5;
    vec3 inputDistort = texture(sceneSurface, vec2(ux, uy)).rgb;
    color = vec3(inputDistort.r, color.g, color.b);
  }
  
  if (hdr.bInterleaveVideo.x >= 1.0) {
    float time = gWorldBuffer.fEngineTime;
    vec2 resolution = vec2(gWorldBuffer.screenSize.xy);
    float interval = hdr.interleaveShakeInterval.x;
    float strength = smoothstep(interval * 0.5, interval, interval - mod(time, interval));
    vec2 shake = vec2(strength * 8.0 + 0.5) * vec2(
      random(vec2(time)) * 2.0 - 1.0,
      random(vec2(time * 2.0)) * 2.0 - 1.0
    ) / resolution;

    float y = uv.y * resolution.y;
    float rgbWave = (
        snoise3(vec3(0.0, y * 0.01, time * 400.0)) * (2.0 + strength * 32.0)
        * snoise3(vec3(0.0, y * 0.02, time * 200.0)) * (1.0 + strength * 4.0)
        + step(0.9995, sin(y * 0.005 + time * 1.6)) * 12.0
        + step(0.9999, sin(y * 0.005 + time * 2.0)) * -18.0
      ) / resolution.x;
    float rgbDiff = (6.0 + sin(time * 500.0 + uv.y * 40.0) * (20.0 * strength + 1.0)) / resolution.x;
    float rgbUvX = uv.x + rgbWave;
    float r = texture(sceneSurface, vec2(rgbUvX + rgbDiff, uv.y) + shake).r;
    float g = texture(sceneSurface, vec2(rgbUvX, uv.y) + shake).g;
    float b = texture(sceneSurface, vec2(rgbUvX - rgbDiff, uv.y) + shake).b;

    float whiteNoise = (random(uv + mod(time, 10.0)) * 2.0 - 1.0) * (0.15 + strength * 0.15);

    float bnTime = floor(time * 20.0) * 200.0;
    float noiseX = step((snoise3(vec3(0.0, uv.x * 3.0, bnTime)) + 1.0) / 2.0, 0.12 + strength * 0.3);
    float noiseY = step((snoise3(vec3(0.0, uv.y * 3.0, bnTime)) + 1.0) / 2.0, 0.12 + strength * 0.3);
    float bnMask = noiseX * noiseY;
    float bnUvX = uv.x + sin(bnTime) * 0.2 + rgbWave;
    float bnR = texture(sceneSurface, vec2(bnUvX + rgbDiff, uv.y)).r * bnMask;
    float bnG = texture(sceneSurface, vec2(bnUvX, uv.y)).g * bnMask;
    float bnB = texture(sceneSurface, vec2(bnUvX - rgbDiff, uv.y)).b * bnMask;
    vec4 blockNoise = vec4(bnR, bnG, bnB, 1.0);

    float bnTime2 = floor(time * 25.0) * 300.0;
    float noiseX2 = step((snoise3(vec3(0.0, uv.x * 2.0, bnTime2)) + 1.0) / 2.0, 0.12 + strength * 0.5);
    float noiseY2 = step((snoise3(vec3(0.0, uv.y * 8.0, bnTime2)) + 1.0) / 2.0, 0.12 + strength * 0.3);
    float bnMask2 = noiseX2 * noiseY2;
    float bnR2 = texture(sceneSurface, vec2(bnUvX + rgbDiff, uv.y)).r * bnMask2;
    float bnG2 = texture(sceneSurface, vec2(bnUvX, uv.y)).g * bnMask2;
    float bnB2 = texture(sceneSurface, vec2(bnUvX - rgbDiff, uv.y)).b * bnMask2;
    vec4 blockNoise2 = vec4(bnR2, bnG2, bnB2, 1.0);

    float waveNoise = (sin(uv.y * 1200.0) + 1.0) / 2.0 * (0.15 + strength * 0.2);
    vec4 interleavedColor = vec4(r, g, b, 1.0) * (1.0 - bnMask - bnMask2) + (whiteNoise + blockNoise + blockNoise2 - waveNoise);
    
    color = interleavedColor.rgb;
  }
  
  // Perform an additive blending to the scene surface. This is because
  // we want to be able to enhance bloom areas within the scene texture.
  if (gWorldBuffer.bloomEnabled >= 1) { color += bloom * paramConfigs.bloomStrength; }
  
  // Extended exposure pass with Uncharted 2 tone mapping. Gamma correction
  // is also enabled.
  vec3 tone = Uncharted2Tonemap(color * gWorldBuffer.exposure);
  tone = tone * (1.0 / Uncharted2Tonemap(vec3(11.2)));
  
  // Gamma correction.
  tone = pow(tone, vec3(1.0 / gWorldBuffer.gamma));
  
  vec4 post = vec4(tone, 1.0);
  fragColor = post;
}






