// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 4) out;

// Global const buffer ALWAYS bound to descriptor set 0, or the 
// first descriptor set.
layout (set = 0, binding = 0) uniform GlobalBuffer {
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
  vec4  clipPlane0;
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

layout (set = 1, binding = 0) uniform ParticleBuffer {
  float level[16];
  mat4  model;
  mat4  modelView;
  float fadeAt;
  float fadeThreshold;
  float angleThreshold;
  float rate;
  float lifeTimeScale;
  float particleMaxAlive;
  float maxParticles;
  float isWorldSpace;
} particleBuffer;

in VertOut {
  vec4 color;
  vec4 worldPos;
  float angle;
  float size;
  float weight;
  float life;
} vert_out[];

out FragIn {
  vec4  color;
  vec4  worldPos;
  vec2  uv;
  float angle;
  float life;
} frag_in;

void main()
{
  float size = vert_out[0].size;
  mat4 mv = particleBuffer.modelView;
  vec3 right = vec3(mv[0][0], mv[1][0], mv[2][0]);
  vec3 up = vec3(mv[0][1], mv[1][1], mv[2][1]);
  vec3 P = gl_in[0].gl_Position.xyz;
  mat4 vp = gWorldBuffer.viewProj;
  
  vec3 va = P - (right * up) * size;
  gl_Position = vp * vec4(va, 1.0);
  frag_in.uv = vec2(0.0, 0.0);
  frag_in.color = vert_out[0].color;
  frag_in.life = vert_out[0].life;
  EmitVertex();
  
  vec3 vb = P - (right - up) * size;
  gl_Position = vp * vec4(vb, 1.0);
  frag_in.uv = vec2(0.0, 1.0);
  frag_in.color = vert_out[0].color;
  frag_in.life = vert_out[0].life;
  EmitVertex();
  
  vec3 vd = P + (right - up) * size;
  gl_Position = vp * vec4(vd, 1.0);
  frag_in.uv = vec2(1.0, 0.0);
  frag_in.color = vert_out[0].color;
  frag_in.life = vert_out[0].life;
  EmitVertex();
  
  vec3 vc = P + (right + up) * size;
  gl_Position = vp * vec4(vc, 1.0);
  frag_in.uv = vec2(1.0, 1.0);
  frag_in.color = vert_out[0].color;
  frag_in.life = vert_out[0].life;
  EmitVertex();
  
  EndPrimitive();
}
