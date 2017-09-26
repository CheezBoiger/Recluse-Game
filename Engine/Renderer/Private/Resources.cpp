// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Resources.hpp"

#include "RHI/ComputePipeline.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "RHI/FrameBuffer.hpp"

#include "Core/Exception.hpp"

#include <unordered_map>
#include <algorithm>


namespace Recluse {


std::unordered_map<resource_id_t, GraphicsPipeline* > GraphicsPipelineMap;
std::unordered_map<resource_id_t, ComputePipeline* >  ComputePipelineMap;
std::unordered_map<resource_id_t, FrameBuffer* > FrameBuffers;


Resources& gResources()
{
  static Resources resources;
  return resources;
}


resource_id_t Resources::RegisterGraphicsPipeline(std::string name, GraphicsPipeline* pipeline)
{
  resource_id_t uid = std::hash<std::string>()(name);
  
  if (GraphicsPipelineMap.find(uid) == GraphicsPipelineMap.end()) {
    GraphicsPipelineMap[uid] = pipeline;    
  } else {
    uid = 0;
  }

  return uid;
}


resource_id_t Resources::RegisterComputePipeline(std::string name, ComputePipeline* pipeline)
{
  resource_id_t uid = std::hash<std::string>()(name);
  if (ComputePipelineMap.find(uid) == ComputePipelineMap.end()) {
    ComputePipelineMap[uid] = pipeline;
  } else {
    uid = 0;
  }
  
  return uid;
}


resource_id_t Resources::RegisterFrameBuffer(std::string name, FrameBuffer* framebuffer)
{
  resource_id_t uid = std::hash<std::string>()(name);
  if (FrameBuffers.find(uid) == FrameBuffers.end()) {
    FrameBuffers[uid] = framebuffer;
  } else {
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
} // Recluse