// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "VertexDescription.hpp"
#include "Core/Exception.hpp"


namespace Recluse {

VkVertexInputBindingDescription SkinnedVertexDescription::GetBindingDescription()
{
  VkVertexInputBindingDescription binding = { };
  binding.binding = 0;
  binding.stride = sizeof(SkinnedVertex);
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return binding;
}


std::vector<VkVertexInputAttributeDescription> SkinnedVertexDescription::GetVertexAttributes()
{
  std::vector<VkVertexInputAttributeDescription> attribs(6);

  u32 offset = 0;
  attribs[0].binding = 0;
  attribs[0].location = 0;
  attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attribs[0].offset = offset;
  offset += sizeof(r32) * 4;

  attribs[1].binding = 0;
  attribs[1].location = 1;
  attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attribs[1].offset = offset;
  offset += sizeof(r32) * 4;

  attribs[2].binding = 0;
  attribs[2].location = 2;
  attribs[2].format = VK_FORMAT_R32G32_SFLOAT;
  attribs[2].offset = offset;
  offset += sizeof(r32) * 2;

  attribs[3].binding = 0;
  attribs[3].location = 3;
  attribs[3].format = VK_FORMAT_R32G32_SFLOAT;
  attribs[3].offset = offset;
  offset += sizeof(r32) * 2;

  attribs[4].binding = 0;
  attribs[4].location = 5;
  attribs[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attribs[4].offset = offset;
  offset += sizeof(r32) * 4;

  attribs[5].binding = 0;
  attribs[5].location = 6;
  attribs[5].format = VK_FORMAT_R32G32B32A32_SINT;
  attribs[5].offset = offset;
  offset += sizeof(i32) * 4;

  return attribs;
}


VkVertexInputBindingDescription QuadVertexDescription::GetBindingDescription()
{
  VkVertexInputBindingDescription binding = { };
  binding.binding = 0;
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  binding.stride = sizeof(QuadVertex);
  return binding;
}


std::vector<VkVertexInputAttributeDescription> QuadVertexDescription::GetVertexAttributes()
{
  std::vector<VkVertexInputAttributeDescription> attribs(2);

  attribs[0].binding = 0;
  attribs[0].format = VK_FORMAT_R32G32_SFLOAT;
  attribs[0].location = 0;
  attribs[0].offset = 0;

  attribs[1].binding = 0;
  attribs[1].format = VK_FORMAT_R32G32_SFLOAT;
  attribs[1].location = 1;
  attribs[1].offset = sizeof(r32) * 2;
  return attribs;
}


VkVertexInputBindingDescription StaticVertexDescription::GetBindingDescription()
{
  VkVertexInputBindingDescription binding = { };
  binding.binding = 0;
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  binding.stride = sizeof(StaticVertex);
  return binding;
}


std::vector<VkVertexInputAttributeDescription> StaticVertexDescription::GetVertexAttributes()
{
  std::vector<VkVertexInputAttributeDescription> attribs(4);
  u32 offset = 0;

  attribs[0].binding = 0;
  attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attribs[0].location = 0;
  attribs[0].offset = offset;
  offset += sizeof(Vector4);

  attribs[1].binding = 0;
  attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attribs[1].location = 1;
  attribs[1].offset = offset;
  offset += sizeof(Vector4);

  attribs[2].binding = 0;
  attribs[2].format = VK_FORMAT_R32G32_SFLOAT;
  attribs[2].location = 2;
  attribs[2].offset = offset;
  offset += sizeof(Vector2);

  attribs[3].binding = 0;
  attribs[3].format = VK_FORMAT_R32G32_SFLOAT;
  attribs[3].location = 3;
  attribs[3].offset = offset;
  offset += sizeof(Vector2);
  
  return attribs;
}
} // Recluse