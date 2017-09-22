// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Resources.hpp"

#include "RHI/ComputePipeline.hpp"
#include "RHI/GraphicsPipeline.hpp"

#include "Core/Exception.hpp"

#include <unordered_map>
#include <algorithm>


namespace Recluse {

std::unordered_map<resource_id_t, GraphicsPipeline* > GraphicsPipelineMap;
std::unordered_map<resource_id_t, ComputePipeline* >  ComputePipelineMap;



Resources& Resources::Global()
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
} // Recluse