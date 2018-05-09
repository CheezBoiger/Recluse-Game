// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "Resources.hpp"
#include "RenderObject.hpp"

#include "RHI/ComputePipeline.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/DescriptorSet.hpp"
#include "RHI/FrameBuffer.hpp"
#include "RHI/Texture.hpp"

#include "Core/Exception.hpp"


#include <unordered_map>
#include <algorithm>


namespace Recluse {


std::unordered_map<std::string, GraphicsPipeline* > GraphicsPipelineMap;
std::unordered_map<std::string, ComputePipeline* >  ComputePipelineMap;
std::unordered_map<std::string, FrameBuffer* > FrameBuffers;
std::unordered_map<std::string, Texture*> RenderTextureMap;
std::unordered_map<std::string, Sampler*> SamplerMap;
std::unordered_map<std::string, DescriptorSetLayout*> DescriptorSetLayoutMap;
std::unordered_map<std::string, DescriptorSet* > DescriptorSetMap;
std::unordered_map<uuid64, RenderObject*> RenderObjects;

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


b32 Resources::RegisterGraphicsPipeline(std::string str, GraphicsPipeline* pipeline)
{ 
  if (GraphicsPipelineMap.find(str) == GraphicsPipelineMap.end()) {
    GraphicsPipelineMap[str] = pipeline;
    return true;    
  }

  return false;
}


b32 Resources::RegisterComputePipeline(std::string str, ComputePipeline* pipeline)
{
  if (ComputePipelineMap.find(str) == ComputePipelineMap.end()) {
    ComputePipelineMap[str] = pipeline; 
    return true;
  }
  
  return false;
}


b32 Resources::RegisterFrameBuffer(std::string str, FrameBuffer* framebuffer)
{
  if (FrameBuffers.find(str) == FrameBuffers.end()) {
    FrameBuffers[str] = framebuffer;
    return true;
  }

  return false;
}


b32 Resources::RegisterRenderTexture(std::string str, Texture* texture)
{
  if (RenderTextureMap.find(str) == RenderTextureMap.end()) {
    RenderTextureMap[str] = texture;
    return true;
  }
  return false;
}


b32 Resources::RegisterSampler(std::string str, Sampler* sampler)
{
  if (SamplerMap.find(str) == SamplerMap.end()) {
    SamplerMap[str] = sampler;
    return true;
  }

  return false;
}


b32 Resources::RegisterDescriptorSetLayout(std::string str, DescriptorSetLayout* layout)
{
  if (DescriptorSetLayoutMap.find(str) == DescriptorSetLayoutMap.end()) {
    DescriptorSetLayoutMap[str] = layout;
    return true;
  }
  return false;
}


b32 Resources::RegisterDescriptorSet(std::string str, DescriptorSet* set)
{
  if (DescriptorSetMap.find(str) == DescriptorSetMap.end()) {
    DescriptorSetMap[str] = set;
    return true;
  }
  return false;
}


GraphicsPipeline* Resources::GetGraphicsPipeline(std::string str)
{
  if (GraphicsPipelineMap.find(str) != GraphicsPipelineMap.end()) {
    return GraphicsPipelineMap[str];
  }
  
  return nullptr;
}


ComputePipeline* Resources::GetComputePipeline(std::string str)
{
  if (ComputePipelineMap.find(str) != ComputePipelineMap.end()) {
    return ComputePipelineMap[str];
  } 

  return nullptr;
}


FrameBuffer* Resources::GetFrameBuffer(std::string str)
{
  if (FrameBuffers.find(str) != FrameBuffers.end()) {
    return FrameBuffers[str];
  }
  return nullptr;
}


Texture* Resources::GetRenderTexture(std::string str)
{
  if (RenderTextureMap.find(str) != RenderTextureMap.end()) {
    return RenderTextureMap[str];
  }
  return nullptr;
}


Sampler* Resources::GetSampler(std::string str)
{
  if (SamplerMap.find(str) != SamplerMap.end()) {
    return SamplerMap[str];
  }
  return nullptr;
}


DescriptorSetLayout* Resources::GetDescriptorSetLayout(std::string str)
{
  if (DescriptorSetLayoutMap.find(str) != DescriptorSetLayoutMap.end()) {
    return DescriptorSetLayoutMap[str];
  }
  return nullptr;
}


DescriptorSet* Resources::GetDescriptorSet(std::string str)
{
  if (DescriptorSetMap.find(str) != DescriptorSetMap.end()) {
    return DescriptorSetMap[str];
  }
  return nullptr;
}


GraphicsPipeline* Resources::UnregisterGraphicsPipeline(std::string str)
{
  GraphicsPipeline* pipeline = nullptr;

  if (GraphicsPipelineMap.find(str) != GraphicsPipelineMap.end()) {
    pipeline = GraphicsPipelineMap[str];
    GraphicsPipelineMap.erase(str);
  }
  
  return pipeline;
}


ComputePipeline* Resources::UnregisterComputePipeline(std::string str)
{
  ComputePipeline* pipeline = nullptr;
  if (ComputePipelineMap.find(str) != ComputePipelineMap.end()) {
    pipeline = ComputePipelineMap[str];
    ComputePipelineMap.erase(str);
  }

  return pipeline;
}


FrameBuffer* Resources::UnregisterFrameBuffer(std::string str)
{
  FrameBuffer* framebuffer = nullptr;
  
  if (FrameBuffers.find(str) != FrameBuffers.end()) {
    framebuffer = FrameBuffers[str];
    FrameBuffers.erase(str);
  }  

  return framebuffer;
}


Texture* Resources::UnregisterRenderTexture(std::string str)
{
  Texture* texture = nullptr;

  if (RenderTextureMap.find(str) != RenderTextureMap.end()) {
    texture = RenderTextureMap[str];
    RenderTextureMap.erase(str);
  }

  return texture;
}


Sampler* Resources::UnregisterSampler(std::string str)
{
  Sampler* sampler = nullptr;

  if (SamplerMap.find(str) != SamplerMap.end()) {
    sampler = SamplerMap[str];
    SamplerMap.erase(str);
  }

  return sampler;
}


DescriptorSetLayout* Resources::UnregisterDescriptorSetLayout(std::string str)
{
  DescriptorSetLayout* layout = nullptr;

  if (DescriptorSetLayoutMap.find(str) != DescriptorSetLayoutMap.end()) {
    layout = DescriptorSetLayoutMap[str];
    DescriptorSetLayoutMap.erase(str);
  }

  return layout;
}


DescriptorSet* Resources::UnregisterDescriptorSet(std::string str)
{
  DescriptorSet* set = nullptr;

  if (DescriptorSetMap.find(str) != DescriptorSetMap.end()) {
    set = DescriptorSetMap[str];
    DescriptorSetMap.erase(str);
  }

  return set;
}


RenderObject* Resources::GetRenderObject(uuid64 uuid)
{
  RenderObject* obj = nullptr;
  if (RenderObjects.find(uuid) != RenderObjects.end()) {
    obj = RenderObjects[uuid];
  }
  return obj;
}


RenderObject* Resources::UnregisterRenderObject(uuid64 uuid)
{
  RenderObject* obj = nullptr;
  if (RenderObjects.find(uuid) != RenderObjects.end()) {
    obj = RenderObjects[uuid];
    RenderObjects.erase(uuid);
  }
  return obj;
}


b32  Resources::RegisterRenderObject(RenderObject* obj)
{
  if (!obj) return false;
  if (RenderObjects.find(obj->GetUUID()) == RenderObjects.end()) {
    RenderObjects[obj->GetUUID()] = obj;
    return true;
  }
  return false;
}
} // Recluse