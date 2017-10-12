// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class GraphicsPipeline;
class ComputePipeline;
class FrameBuffer;
class Texture;
class Sampler;
class DescriptorSetLayout;
class DescriptorSet;
class Material;
class Mesh;

typedef size_t resource_id_t;


// Resources handler, which maps out our pipelines and shaders and whatnot.
// TODO(): We need to make sure we maintain cache on resources, I don't very much
// like using strings as our key for these. Maybe int values?
class Resources {
  static resource_id_t idCount;
  resource_id_t ObtainResourceId();

public:
  GraphicsPipeline*     GetGraphicsPipeline(std::string str);
  ComputePipeline*      GetComputePipeline(std::string str);
  FrameBuffer*          GetFrameBuffer(std::string str);
  Texture*              GetRenderTexture(std::string str);
  Sampler*              GetSampler(std::string str);
  DescriptorSetLayout*  GetDescriptorSetLayout(std::string str);
  DescriptorSet*        GetDescriptorSet(std::string str);

  b8                    RegisterGraphicsPipeline(std::string str, GraphicsPipeline* pipeline);
  b8                    RegisterComputePipeline(std::string str, ComputePipeline* pipeline);
  b8                    RegisterFrameBuffer(std::string str, FrameBuffer* framebuffer);
  b8                    RegisterRenderTexture(std::string str, Texture* texture);
  b8                    RegisterSampler(std::string str, Sampler* sampler);
  b8                    RegisterDescriptorSetLayout(std::string str, DescriptorSetLayout* layout);
  b8                    RegisterDescriptorSet(std::string str, DescriptorSet* set);
  
  GraphicsPipeline*     UnregisterGraphicsPipeline(std::string str);
  ComputePipeline*      UnregisterComputePipeline(std::string str);
  FrameBuffer*          UnregisterFrameBuffer(std::string str);
  Texture*              UnregisterRenderTexture(std::string str);
  Sampler*              UnregisterSampler(std::string str);
  DescriptorSetLayout*  UnregisterDescriptorSetLayout(std::string str);
  DescriptorSet*        UnregisterDescriptorSet(std::string str);
};


Resources& gResources();
} // Recluse