// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "Resources.hpp"

#include "RHI/ComputePipeline.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/FrameBuffer.hpp"
#include "RHI/Texture.hpp"

#include "Core/Exception.hpp"


#include <unordered_map>
#include <algorithm>


namespace Recluse {


std::unordered_map<u64, GraphicsPipeline* > GraphicsPipelineMap;
std::unordered_map<u64, ComputePipeline* >  ComputePipelineMap;
std::unordered_map<u64, FrameBuffer* > FrameBuffers;
std::unordered_map<u64, Texture*> RenderTextureMap;
std::unordered_map<u64, Sampler*> SamplerMap;
std::unordered_map<u64, DescriptorSetLayout*> DescriptorSetLayoutMap;
std::unordered_map<u64, DescriptorSet* > DescriptorSetMap;

resource_id_t Resources::idCount = 0;


resource_id_t Resources::ObtainResourceId()
{
  resource_id_t id = idCount++;
  return id;
}


Resources& gResources()
{
  static Resources resources;
  return resources;
}


b32 Resources::RegisterGraphicsPipeline(GraphicsPipeline* pipeline)
{ 
  if (!pipeline) return false;
  graphics_uuid_t id = pipeline->GetUUID();
  if (GraphicsPipelineMap.find(id) == GraphicsPipelineMap.end()) {
    GraphicsPipelineMap[id] = pipeline;
    return true;    
  }

  return false;
}


b32 Resources::RegisterComputePipeline(ComputePipeline* pipeline)
{
  graphics_uuid_t id = pipeline->GetUUID();
  if (ComputePipelineMap.find(id) == ComputePipelineMap.end()) {
    ComputePipelineMap[id] = pipeline; 
    return true;
  }
  
  return false;
}


b32 Resources::RegisterFrameBuffer(FrameBuffer* framebuffer)
{
  graphics_uuid_t id = framebuffer->GetUUID();
  if (FrameBuffers.find(id) == FrameBuffers.end()) {
    FrameBuffers[id] = framebuffer;
    return true;
  }

  return false;
}


b32 Resources::RegisterRenderTexture(Texture* texture)
{
  graphics_uuid_t id = texture->GetUUID();
  if (RenderTextureMap.find(id) == RenderTextureMap.end()) {
    RenderTextureMap[id] = texture;
    return true;
  }
  return false;
}


b32 Resources::RegisterSampler(Sampler* sampler)
{
  graphics_uuid_t id = sampler->GetUUID();
  if (SamplerMap.find(id) == SamplerMap.end()) {
    SamplerMap[id] = sampler;
    return true;
  }

  return false;
}


b32 Resources::RegisterDescriptorSetLayout(DescriptorSetLayout* layout)
{
  graphics_uuid_t id = layout->GetUUID();
  if (DescriptorSetLayoutMap.find(id) == DescriptorSetLayoutMap.end()) {
    DescriptorSetLayoutMap[id] = layout;
    return true;
  }
  return false;
}


b32 Resources::RegisterDescriptorSet(DescriptorSet* set)
{
  graphics_uuid_t id = set->GetUUID();
  if (DescriptorSetMap.find(id) == DescriptorSetMap.end()) {
    DescriptorSetMap[id] = set;
    return true;
  }
  return false;
}


GraphicsPipeline* Resources::GetGraphicsPipeline(u64 id)
{
  if (GraphicsPipelineMap.find(id) != GraphicsPipelineMap.end()) {
    return GraphicsPipelineMap[id];
  }
  
  return nullptr;
}


ComputePipeline* Resources::GetComputePipeline(u64 id)
{
  if (ComputePipelineMap.find(id) != ComputePipelineMap.end()) {
    return ComputePipelineMap[id];
  } 

  return nullptr;
}


FrameBuffer* Resources::GetFrameBuffer(u64 id)
{
  if (FrameBuffers.find(id) != FrameBuffers.end()) {
    return FrameBuffers[id];
  }
  return nullptr;
}


Texture* Resources::GetRenderTexture(u64 id)
{
  if (RenderTextureMap.find(id) != RenderTextureMap.end()) {
    return RenderTextureMap[id];
  }
  return nullptr;
}


Sampler* Resources::GetSampler(u64 id)
{
  if (SamplerMap.find(id) != SamplerMap.end()) {
    return SamplerMap[id];
  }
  return nullptr;
}


DescriptorSetLayout* Resources::GetDescriptorSetLayout(u64 id)
{
  if (DescriptorSetLayoutMap.find(id) != DescriptorSetLayoutMap.end()) {
    return DescriptorSetLayoutMap[id];
  }
  return nullptr;
}


DescriptorSet* Resources::GetDescriptorSet(u64 id)
{
  if (DescriptorSetMap.find(id) != DescriptorSetMap.end()) {
    return DescriptorSetMap[id];
  }
  return nullptr;
}


GraphicsPipeline* Resources::UnregisterGraphicsPipeline(u64 id)
{
  GraphicsPipeline* pipeline = nullptr;

  if (GraphicsPipelineMap.find(id) != GraphicsPipelineMap.end()) {
    pipeline = GraphicsPipelineMap[id];
    GraphicsPipelineMap.erase(id);
  }
  
  return pipeline;
}


ComputePipeline* Resources::UnregisterComputePipeline(u64 id)
{
  ComputePipeline* pipeline = nullptr;
  if (ComputePipelineMap.find(id) != ComputePipelineMap.end()) {
    pipeline = ComputePipelineMap[id];
    ComputePipelineMap.erase(id);
  }

  return pipeline;
}


FrameBuffer* Resources::UnregisterFrameBuffer(u64 id)
{
  FrameBuffer* framebuffer = nullptr;
  
  if (FrameBuffers.find(id) != FrameBuffers.end()) {
    framebuffer = FrameBuffers[id];
    FrameBuffers.erase(id);
  }  

  return framebuffer;
}


Texture* Resources::UnregisterRenderTexture(u64 id)
{
  Texture* texture = nullptr;

  if (RenderTextureMap.find(id) != RenderTextureMap.end()) {
    texture = RenderTextureMap[id];
    RenderTextureMap.erase(id);
  }

  return texture;
}


Sampler* Resources::UnregisterSampler(u64 id)
{
  Sampler* sampler = nullptr;

  if (SamplerMap.find(id) != SamplerMap.end()) {
    sampler = SamplerMap[id];
    SamplerMap.erase(id);
  }

  return sampler;
}


DescriptorSetLayout* Resources::UnregisterDescriptorSetLayout(u64 id)
{
  DescriptorSetLayout* layout = nullptr;

  if (DescriptorSetLayoutMap.find(id) != DescriptorSetLayoutMap.end()) {
    layout = DescriptorSetLayoutMap[id];
    DescriptorSetLayoutMap.erase(id);
  }

  return layout;
}


DescriptorSet* Resources::UnregisterDescriptorSet(u64 id)
{
  DescriptorSet* set = nullptr;

  if (DescriptorSetMap.find(id) != DescriptorSetMap.end()) {
    set = DescriptorSetMap[id];
    DescriptorSetMap.erase(id);
  }

  return set;
}
} // Recluse