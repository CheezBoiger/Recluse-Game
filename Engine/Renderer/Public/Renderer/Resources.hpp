// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
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
class MaterialDescriptor;
class Mesh;
class RenderObject;

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
  RenderObject*         GetRenderObject(uuid64 uuid);

  b32                   RegisterGraphicsPipeline(std::string str, GraphicsPipeline* pipeline);
  b32                   RegisterComputePipeline(std::string str, ComputePipeline* pipeline);
  b32                   RegisterFrameBuffer(std::string str, FrameBuffer* framebuffer);
  b32                   RegisterRenderTexture(std::string str, Texture* texture);
  b32                   RegisterSampler(std::string str, Sampler* sampler);
  b32                   RegisterDescriptorSetLayout(std::string str, DescriptorSetLayout* layout);
  b32                   RegisterDescriptorSet(std::string str, DescriptorSet* set);

  b32                    RegisterRenderObject(RenderObject* obj);
  
  GraphicsPipeline*     UnregisterGraphicsPipeline(std::string str);
  ComputePipeline*      UnregisterComputePipeline(std::string str);
  FrameBuffer*          UnregisterFrameBuffer(std::string str);
  Texture*              UnregisterRenderTexture(std::string str);
  Sampler*              UnregisterSampler(std::string str);
  DescriptorSetLayout*  UnregisterDescriptorSetLayout(std::string str);
  DescriptorSet*        UnregisterDescriptorSet(std::string str);
  RenderObject*         UnregisterRenderObject(uuid64 uuid);
};


Resources& gResources();
} // Recluse