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

typedef size_t resource_id_t;


// Resources handler, which maps out our pipelines and shaders and whatnot.
// TODO(): We need to make sure we maintain cache on resources, I don't very much
// like using strings as our key for these. Maybe int values?
class Resources {
  static resource_id_t idCount;
  resource_id_t ObtainResourceId();

public:
  GraphicsPipeline*     GetGraphicsPipeline(u64 id);
  ComputePipeline*      GetComputePipeline(u64 id);
  FrameBuffer*          GetFrameBuffer(u64 id);
  Texture*              GetRenderTexture(u64 id);
  Sampler*              GetSampler(u64 id);
  DescriptorSetLayout*  GetDescriptorSetLayout(u64 id);
  DescriptorSet*        getDescriptorSet(u64 id);

  b32                   RegisterGraphicsPipeline(GraphicsPipeline* pipeline);
  b32                   RegisterComputePipeline(ComputePipeline* pipeline);
  b32                   RegisterFrameBuffer(FrameBuffer* framebuffer);
  b32                   RegisterRenderTexture(Texture* texture);
  b32                   RegisterSampler(Sampler* sampler);
  b32                   RegisterDescriptorSetLayout(DescriptorSetLayout* layout);
  b32                   RegisterDescriptorSet(DescriptorSet* set);
  
  GraphicsPipeline*     UnregisterGraphicsPipeline(u64 id);
  ComputePipeline*      UnregisterComputePipeline(u64 id);
  FrameBuffer*          UnregisterFrameBuffer(u64 id);
  Texture*              UnregisterRenderTexture(u64 id);
  Sampler*              UnregisterSampler(u64 id);
  DescriptorSetLayout*  UnregisterDescriptorSetLayout(u64 id);
  DescriptorSet*        UnregisterDescriptorSet(u64 id);
};


Resources& gResources();
} // Recluse