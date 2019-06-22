// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common/Globals.glsl"

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (set = 0, binding = 0) uniform Globals {
  GlobalBuffer global;
} gWorldBuffer;

in VertOut {
  vec4 currWPos;
  vec4 prevWPos;
  vec4 nextWPos;
  vec4 color;
  float radius;
} vert_out[];


out FragIn {
  vec4 color;
} frag_in;


void main()
{
  mat4 vp = gWorldBuffer.global.viewProj;
  vec3 c = gWorldBuffer.global.cameraPos.xyz;
  vec3 pi = vert_out[0].currWPos.xyz;
  vec3 ppi = vert_out[0].nextWPos.xyz;
  vec3 pni = vert_out[0].prevWPos.xyz;
  vec3 zi = normalize(c - pi);
  vec3 ti = normalize(ppi - pni);
  float r = vert_out[0].radius;
  
  gl_Position = vp * vec4(pi + cross(ti, zi) * r, 1.0);
  frag_in.color = vert_out[0].color;
  EmitVertex();
  
  gl_Position = vp * vec4(pi - cross(ti, zi) * r, 1.0);
  frag_in.color = vert_out[0].color;
  EmitVertex();
  
  vec3 tni = normalize(pi - pni);
  vec3 zni = normalize(c - pni);
  gl_Position = vp * vec4(pni + cross(tni, zni) * r, 1.0);
  frag_in.color = vert_out[0].color;
  EmitVertex();
  
  gl_Position = vp * vec4(pni - cross(tni, zni) * r, 1.0);
  frag_in.color = vert_out[0].color;
  EmitVertex();
  
  EndPrimitive();
}