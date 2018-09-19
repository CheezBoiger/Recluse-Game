// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable

layout (location = 0) in vec4   position;
layout (location = 1) in vec4   normal;
layout (location = 2) in vec2   texcoord0;
layout (location = 3) in vec2   texcoord1;

#if defined(INCLUDE_MORPH_TARGET_ANIMATION)
// For morph target animation, we must define our position layout, above, as the 
// default pose (or the bind pose.) Source pose is defined by this layout.
layout (location = 4) in vec4   position0;
layout (location = 5) in vec4   normal0;
layout (location = 6) in vec2   uv00;
layout (location = 7) in vec2   uv10;

// Destination pose is defined here.
layout (location = 8) in vec4  position1;
layout (location = 9) in vec4  normal1;
layout (location = 10) in vec2  uv01;
layout (location = 11) in vec2  uv11;
#endif


layout (set = 0, binding = 0) uniform ObjectBuffer {
  mat4  model;
  mat4  normalMatrix;
  float lod;
  int   hasBones; 
  float w0;
  float w1;
} obj_buffer;


// One directional light.
layout (set = 1, binding = 0) uniform LightSpace {
  mat4  viewProj;
} light_space;

out FragIn {
  vec2 uv0;
  vec2 uv1;
} fragIn;

void main()
{
  vec4 worldPosition = position;
  vec2 temp_uv0 = texcoord0;
  vec2 temp_uv1 = texcoord1;
  
#if defined(INCLUDE_MORPH_TARGET_ANIMATION)
  float w0 = obj_buffer.w0;
  float w1 = obj_buffer.w1;
  vec4  morphPositionDiff0 = position0 - worldPosition;
  vec4  morphPositionDiff1 = position1 - worldPosition;
  vec2  morphUV00 = uv00 - temp_uv0;
  vec2  morphUV01 = uv01 - temp_uv0;
  vec2  morphUV10 = uv10 - temp_uv1;
  vec2  morphUV11 = uv11 - temp_uv1;
  worldPosition += morphPositionDiff0 * w0;
  worldPosition += morphPositionDiff1 * w1;
  temp_uv0 += morphUV00 * w0;
  temp_uv0 += morphUV01 * w1;
  temp_uv1 += morphUV10 * w0;
  temp_uv1 += morphUV11 * w1;
#endif

  mat4 mvp = light_space.viewProj * obj_buffer.model;
  gl_Position = mvp * vec4(worldPosition.xyz, 1.0);
  
  fragIn.uv0 = temp_uv0;
  fragIn.uv1 = temp_uv1;
}