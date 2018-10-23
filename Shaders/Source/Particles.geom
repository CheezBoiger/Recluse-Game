// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

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
  float zNear;
  float zFar;
  int   bloomEnabled;
  int   enableShadows;
  int   enableAA;
} gWorldBuffer;

layout (set = 1, binding = 0) uniform ParticleBuffer {
  float level[16];
  mat4  model;
  mat4  modelView;
  vec4  hasTexture;
  vec4  globalScale;
  vec4  lightFactor;
  vec4  angleRate;
  vec4  fadeIn;
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

#define CONST_PI                3.141592653589793238462643383279502884197169399375
#define DEG_TO_RAD(deg)         ((deg) * (CONST_PI / 180.0))
#define ROTATE_VEC2(a, xrot, yrot)    vec2(a.x * xrot - a.y * yrot, a.x * yrot + a.y * xrot)
void main()
{
  for (int i = 0; i < gl_in.length(); ++i) {
    float size = vert_out[i].size * particleBuffer.globalScale.x;
    vec4 P = gl_in[i].gl_Position;
    mat4 p = gWorldBuffer.proj;
    float xrot = cos(DEG_TO_RAD(vert_out[i].angle));
    float yrot = sin(DEG_TO_RAD(vert_out[i].angle));
    vec2 rotation = vec2(xrot, yrot);
    
    vec2 va = vec2(-0.5, -0.5); 
    va = P.xy + ROTATE_VEC2(va, xrot, yrot) * size;
    gl_Position = p * vec4(va, P.zw);
    frag_in.uv = vec2(0.0, 0.0);
    frag_in.color = vert_out[i].color;
    frag_in.life = vert_out[i].life;
    EmitVertex();
    
    vec2 vb = vec2(-0.5, 0.5);
    vb = P.xy + ROTATE_VEC2(vb, xrot, yrot) * size;
    gl_Position = p * vec4(vb, P.zw);
    frag_in.uv = vec2(0.0, 1.0);
    frag_in.color = vert_out[i].color;
    frag_in.life = vert_out[i].life;
    EmitVertex();
    
    vec2 vd = vec2(0.5, -0.5);
    vd = P.xy + ROTATE_VEC2(vd, xrot, yrot) * size;
    gl_Position = p * vec4(vd, P.zw);
    frag_in.uv = vec2(1.0, 0.0);
    frag_in.color = vert_out[i].color;
    frag_in.life = vert_out[i].life;
    EmitVertex();
    
    vec2 vc = vec2(0.5, 0.5);
    vc = P.xy + ROTATE_VEC2(vc, xrot, yrot) * size;
    gl_Position = p * vec4(vc, P.zw);
    frag_in.uv = vec2(1.0, 1.0);
    frag_in.color = vert_out[i].color;
    frag_in.life = vert_out[i].life;
    EmitVertex();
    
    EndPrimitive();
  }
}
