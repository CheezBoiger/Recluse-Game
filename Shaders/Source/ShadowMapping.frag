// Copyright (c) 2017 Recluse Project. All rights reserved.
#version 430
#extension GL_ARB_separate_shader_objects   : enable
#extension GL_ARB_shading_language_420pack  : enable


layout (location = 0) out vec4 fragColor;



void main()
{
  gl_FragDepth = gl_FragCoord.z;  
  // Incase of debug, we can set a color buffer to this shader to visualize our shadow map.
  fragColor = vec4(0.0, 0.0, 1.0, 1.0);
}