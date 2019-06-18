// Copyright (c) 2017 Recluse Project. All rights reserved.
#ifndef INTERLEAVING_GLSL
#define INTERLEAVING_GLSL

#include "Noise.glsl"

vec4 GenerateInterleave(in sampler2D sceneSurface,
                        vec2 uv, 
                        vec2 resolution, 
                        float interval, 
                        float time)
{
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
  return interleavedColor;
}

#endif // INTERLEAVING_GLSL