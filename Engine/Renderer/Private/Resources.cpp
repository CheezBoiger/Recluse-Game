// Copyright (c) 2017 Recluse Project. All rights reserved.
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


std::unordered_map<resource_id_t, GraphicsPipeline* > GraphicsPipelineMap;
std::unordered_map<resource_id_t, ComputePipeline* >  ComputePipelineMap;
std::unordered_map<resource_id_t, FrameBuffer* > FrameBuffers;
std::unordered_map<resource_id_t, Texture*> RenderTextureMap;
std::unordered_map<resource_id_t, Sampler*> SamplerMap;
std::unordered_map<resource_id_t, DescriptorSetLayout*> DescriptorSetLayoutMap;

resource_id_t Resources::idCount = 0;


resource_id_t Resources::ObtainResourceId()
{
  resource_id_t id = ++idCount;
  return id;
}

Resources& gResources()
{
  static Resources resources;
  return resources;
}


resource_id_t Resources::RegisterGraphicsPipeline(GraphicsPipeline* pipeline)
{
  resource_id_t uid = ObtainResourceId();
  
  if (GraphicsPipelineMap.find(uid) == GraphicsPipelineMap.end()) {
    GraphicsPipelineMap[uid] = pipeline;    
  } else {
    uid = 0;
  }

  return uid;
}


resource_id_t Resources::RegisterComputePipeline(ComputePipeline* pipeline)
{
  resource_id_t uid = ObtainResourceId();
  if (ComputePipelineMap.find(uid) == ComputePipelineMap.end()) {
    ComputePipelineMap[uid] = pipeline;
  } else {
    uid = 0;
  }
  
  return uid;
}


resource_id_t Resources::RegisterFrameBuffer(FrameBuffer* framebuffer)
{
  resource_id_t uid = ObtainResourceId();
  if (FrameBuffers.find(uid) == FrameBuffers.end()) {
    FrameBuffers[uid] = framebuffer;
  } else {
    uid = 0;
  }

  return uid;
}


resource_id_t Resources::RegisterRenderTexture(Texture* texture)
{
  resource_id_t uid = ObtainResourceId();
  if (RenderTextureMap.find(uid) == RenderTextureMap.end()) {
    RenderTextureMap[uid] = texture;
  }
  else {
    uid = 0;
  }

  return uid;
}


resource_id_t Resources::RegisterSampler(Sampler* sampler)
{
  resource_id_t uid = ObtainResourceId();
  if (SamplerMap.find(uid) == SamplerMap.end()) {
    SamplerMap[uid] = sampler;
  }
  else {
    uid = 0;
  }

  return uid;
}


resource_id_t Resources::RegisterDescriptorSetLayout(DescriptorSetLayout* layout)
{
  resource_id_t uid = ObtainResourceId();
  if (DescriptorSetLayoutMap.find(uid) == DescriptorSetLayoutMap.end()) {
    DescriptorSetLayoutMap[uid] = layout;
  }
  else {
    uid = 0;
  }

  return uid;
}


GraphicsPipeline* Resources::GetGraphicsPipeline(resource_id_t uid)
{
  if (GraphicsPipelineMap.find(uid) != GraphicsPipelineMap.end()) {
    return GraphicsPipelineMap[uid];
  }
  
  return nullptr;
}


ComputePipeline* Resources::GetComputePipeline(resource_id_t uid)
{
  if (ComputePipelineMap.find(uid) != ComputePipelineMap.end()) {
    return ComputePipelineMap[uid];
  } 

  return nullptr;
}


FrameBuffer* Resources::GetFrameBuffer(resource_id_t uid)
{
  if (FrameBuffers.find(uid) != FrameBuffers.end()) {
    return FrameBuffers[uid];
  }
  return nullptr;
}


Texture* Resources::GetRenderTexture(resource_id_t uid)
{
  if (RenderTextureMap.find(uid) != RenderTextureMap.end()) {
    return RenderTextureMap[uid];
  }
  return nullptr;
}


Sampler* Resources::GetSampler(resource_id_t uid)
{
  if (SamplerMap.find(uid) != SamplerMap.end()) {
    return SamplerMap[uid];
  }
  return nullptr;
}


DescriptorSetLayout* Resources::GetDescriptorSetLayout(resource_id_t uid)
{
  if (DescriptorSetLayoutMap.find(uid) != DescriptorSetLayoutMap.end()) {
    return DescriptorSetLayoutMap[uid];
  }
  return nullptr;
}


GraphicsPipeline* Resources::UnregisterGraphicsPipeline(resource_id_t uid)
{
  GraphicsPipeline* pipeline = nullptr;

  if (GraphicsPipelineMap.find(uid) != GraphicsPipelineMap.end()) {
    pipeline = GraphicsPipelineMap[uid];
    GraphicsPipelineMap.erase(uid);
  }
  
  return pipeline;
}


ComputePipeline* Resources::UnregisterComputePipeline(resource_id_t uid)
{
  ComputePipeline* pipeline = nullptr;
  if (ComputePipelineMap.find(uid) != ComputePipelineMap.end()) {
    pipeline = ComputePipelineMap[uid];
    ComputePipelineMap.erase(uid);
  }

  return pipeline;
}


FrameBuffer* Resources::UnregisterFrameBuffer(resource_id_t uid)
{
  FrameBuffer* framebuffer = nullptr;
  
  if (FrameBuffers.find(uid) != FrameBuffers.end()) {
    framebuffer = FrameBuffers[uid];
    FrameBuffers.erase(uid);
  }  

  return framebuffer;
}


Texture* Resources::UnregisterRenderTexture(resource_id_t uid)
{
  Texture* texture = nullptr;

  if (RenderTextureMap.find(uid) != RenderTextureMap.end()) {
    texture = RenderTextureMap[uid];
    RenderTextureMap.erase(uid);
  }

  return texture;
}


Sampler* Resources::UnregisterSampler(resource_id_t uid)
{
  Sampler* sampler = nullptr;

  if (SamplerMap.find(uid) != SamplerMap.end()) {
    sampler = SamplerMap[uid];
    SamplerMap.erase(uid);
  }

  return sampler;
}


DescriptorSetLayout* Resources::UnregisterDescriptorSetLayout(resource_id_t uid)
{
  DescriptorSetLayout* layout = nullptr;

  if (DescriptorSetLayoutMap.find(uid) != DescriptorSetLayoutMap.end()) {
    layout = DescriptorSetLayoutMap[uid];
    DescriptorSetLayoutMap.erase(uid);
  }

  return layout;
}
} // Recluse