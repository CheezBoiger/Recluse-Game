// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


in FRAG_IN {
  vec2 position;
  vec2 uv;
} frag_in;


// Out color.
layout (location = 0) out vec4 outColor;

// UI icon used to represent this ui.
layout (set = 0, binding = 0) uniform sampler2D uiMap;

void main()
{
  vec4 sampled = texture(uiMap, frag_in.uv);
    
  // Perform sample and whatnot in the future.
  // Since this is a ui overlay, we are drawing on top of our 
  // rendered scene with blend mode enabled.
  
  outColor = sampled;
}