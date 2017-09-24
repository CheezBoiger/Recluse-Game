#version 430 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 0) in vec2 fragTexCoord;

layout (location = 0) out vec4 fragColor;


layout (binding = 0) uniform sampler2D finalTexture;


void main()
{
  fragColor = texture(finalTexture, fragTexCoord); 
}