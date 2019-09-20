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

layout (set = 1, binding = 0) uniform ParticleBuffer {
  mat4  model;
  mat4  modelView;
  vec4  hasTexture;
  vec4  globalScale;
  vec4  lightFactor;
  vec4  angleRate;
  vec4  fadeIn;
  vec4  animScale;
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
  vec4  clipPos;
  vec2  uv;
  float angle;
  float life;
} frag_in;


in gl_PerVertex {
  vec4 gl_Position;
  float gl_PointSize;
} gl_in[];

out gl_PerVertex {
  vec4 gl_Position;
  float gl_PointSize;
};

#define CONST_PI                3.141592653589793238462643383279502884197169399375
#define DEG_TO_RAD(deg)         ((deg) * (CONST_PI / 180.0))
#define ROTATE_VEC2(a, xrot, yrot)    vec2(a.x * xrot - a.y * yrot, a.x * yrot + a.y * xrot)
void main()
{
  for (int i = 0; i < gl_in.length(); ++i) {
    float size = vert_out[i].size * particleBuffer.globalScale.x;
    vec4 P = gl_in[i].gl_Position;
    mat4 p = gWorldBuffer.global.proj;
    float xrot = cos(DEG_TO_RAD(vert_out[i].angle));
    float yrot = sin(DEG_TO_RAD(vert_out[i].angle));
    vec2 rotation = vec2(xrot, yrot);
    
    vec2 va = vec2(-0.5, -0.5); 
    va = P.xy + ROTATE_VEC2(va, xrot, yrot) * size;
    gl_Position = p * vec4(va, P.zw);
    frag_in.uv = vec2(0.0, 0.0);
    frag_in.color = vert_out[i].color;
    frag_in.life = vert_out[i].life;
    frag_in.clipPos = gl_Position.xyzw;
	gl_PointSize = 1.0;
    EmitVertex();
    
    vec2 vb = vec2(-0.5, 0.5);
    vb = P.xy + ROTATE_VEC2(vb, xrot, yrot) * size;
    gl_Position = p * vec4(vb, P.zw);
    frag_in.uv = vec2(0.0, 1.0);
    frag_in.color = vert_out[i].color;
    frag_in.life = vert_out[i].life;
    frag_in.clipPos = gl_Position.xyzw;
    gl_PointSize = 1.0;
	EmitVertex();
    
    vec2 vd = vec2(0.5, -0.5);
    vd = P.xy + ROTATE_VEC2(vd, xrot, yrot) * size;
    gl_Position = p * vec4(vd, P.zw);
    frag_in.uv = vec2(1.0, 0.0);
    frag_in.color = vert_out[i].color;
    frag_in.life = vert_out[i].life;
    frag_in.clipPos = gl_Position.xyzw;
    gl_PointSize = 1.0;
	EmitVertex();
    
    vec2 vc = vec2(0.5, 0.5);
    vc = P.xy + ROTATE_VEC2(vc, xrot, yrot) * size;
    gl_Position = p * vec4(vc, P.zw);
    frag_in.uv = vec2(1.0, 1.0);
    frag_in.color = vert_out[i].color;
    frag_in.life = vert_out[i].life;
    frag_in.clipPos = gl_Position.xyzw;
    gl_PointSize = 1.0;
	EmitVertex();
    
    EndPrimitive();
  }
}
