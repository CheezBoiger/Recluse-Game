// Copyright (c) 2018 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


out FragIn {
  vec2 uv;
} frag_in;


void main()
{
  // Simple way of rendering a quad without buffers.
  // This algorithm defines a counter clockwise winding order if 
  // culling is enabled. So its front face is counter clockwise.
  frag_in.uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(frag_in.uv * 2.0 + -1.0, 0.0, 1.0);
}