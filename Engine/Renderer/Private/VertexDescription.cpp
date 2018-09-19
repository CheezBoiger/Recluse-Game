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
  attribs[4].location = 4;
  attribs[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attribs[4].offset = offset;
  offset += sizeof(r32) * 4;

  attribs[5].binding = 0;
  attribs[5].location = 5;
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


VkVertexInputBindingDescription UIVertexDescription::GetBindingDescription()
{
  VkVertexInputBindingDescription inputBinding = { };
  inputBinding.binding = 0;
  inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  inputBinding.stride = sizeof(UIVertex);
  return inputBinding;
}


std::vector<VkVertexInputAttributeDescription> UIVertexDescription::GetVertexAttributes()
{
  std::vector<VkVertexInputAttributeDescription> attribs(3);
  u32 offset = 0;

  attribs[0].binding = 0;
  attribs[0].format = VK_FORMAT_R32G32_SFLOAT;
  attribs[0].location = 0;
  attribs[0].offset = offset;
  offset += sizeof(Vector2);

  attribs[1].binding = 0;
  attribs[1].format = VK_FORMAT_R32G32_SFLOAT;
  attribs[1].location = 1;
  attribs[1].offset = offset;
  offset += sizeof(Vector2);

  attribs[2].binding = 0;
  attribs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attribs[2].location = 2;
  attribs[2].offset = offset;
  
  return attribs;
}


std::vector<VkVertexInputBindingDescription> MorphTargetVertexDescription::GetBindingDescriptions(VkVertexInputBindingDescription& input)
{
  std::vector<VkVertexInputBindingDescription> bindings(3);
  bindings[0].binding = input.binding;
  bindings[0].inputRate = input.inputRate;
  bindings[0].stride = input.stride;

  bindings[1].binding = 1;
  bindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  bindings[1].stride = sizeof(MorphVertex);

  bindings[2].binding = 2;
  bindings[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  bindings[2].stride = sizeof(MorphVertex);
  return bindings;
}


size_t GetSize(VkFormat format)
{
  switch (format)
  {
    case VK_FORMAT_R32G32B32A32_SFLOAT: return sizeof(r32) * 4;
    case VK_FORMAT_R32G32B32_SFLOAT: return sizeof(r32) * 3;
    case VK_FORMAT_R32G32_SFLOAT: return sizeof(r32) * 2;
    case VK_FORMAT_R32_SFLOAT:
    default: return sizeof(r32);
  }
}


std::vector<VkVertexInputAttributeDescription> MorphTargetVertexDescription::GetVertexAttributes(std::vector<VkVertexInputAttributeDescription>& attribs)
{
  std::vector<VkVertexInputAttributeDescription> attributes(attribs.size() + 8);
  size_t i = 0;
  size_t offset = 0;
  size_t binding = 0;
  size_t location = 0;
  for (; i < attribs.size(); ++i) {
    attributes[i].binding = attribs[i].binding;
    attributes[i].format = attribs[i].format;
    attributes[i].location = attribs[i].location;
    attributes[i].offset = attribs[i].offset;
    offset = attribs[i].offset + GetSize(attribs[i].format);
    binding = attribs[i].binding;
    location = attribs[i].location;
  }
  
  attributes[i + 0].binding = binding + 1;
  attributes[i + 0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[i + 0].location = location + 1;
  attributes[i + 0].offset = offset;
  offset += sizeof(r32) * 4;

  attributes[i + 1].binding = binding + 1;
  attributes[i + 1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[i + 1].location = location + 2;
  attributes[i + 1].offset = offset;
  offset += sizeof(r32) * 4;

  attributes[i + 2].binding = binding + 1;
  attributes[i + 2].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[i + 2].location = location + 3;
  attributes[i + 2].offset = offset;
  offset += sizeof(r32) * 2;

  attributes[i + 3].binding = binding + 1;
  attributes[i + 3].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[i + 3].location = location + 4;
  attributes[i + 3].offset = offset;
  offset += sizeof(r32) * 2;

  attributes[i + 4].binding = binding + 2;
  attributes[i + 4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[i + 4].location = location + 5;
  attributes[i + 4].offset = offset;
  offset += sizeof(r32) * 4;

  attributes[i + 5].binding = binding + 2;
  attributes[i + 5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[i + 5].location = location + 6;
  attributes[i + 5].offset = offset;
  offset += sizeof(r32) * 4;

  attributes[i + 6].binding = binding + 2;
  attributes[i + 6].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[i + 6].location = location + 7;
  attributes[i + 6].offset = offset;
  offset += sizeof(r32) * 2;

  attributes[i + 7].binding = binding + 2;
  attributes[i + 7].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[i + 7].location = location + 8;
  attributes[i + 7].offset = offset;

  return attributes;
 
}
} // Recluse