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
  GraphicsPipeline*     GetGraphicsPipeline(U64 id);
  ComputePipeline*      GetComputePipeline(U64 id);
  FrameBuffer*          GetFrameBuffer(U64 id);
  Texture*              GetRenderTexture(U64 id);
  Sampler*              getSampler(U64 id);
  DescriptorSetLayout*  GetDescriptorSetLayout(U64 id);
  DescriptorSet*        getDescriptorSet(U64 id);

  B32                   RegisterGraphicsPipeline(GraphicsPipeline* pipeline);
  B32                   RegisterComputePipeline(ComputePipeline* pipeline);
  B32                   RegisterFrameBuffer(FrameBuffer* framebuffer);
  B32                   RegisterRenderTexture(Texture* texture);
  B32                   RegisterSampler(Sampler* sampler);
  B32                   RegisterDescriptorSetLayout(DescriptorSetLayout* layout);
  B32                   RegisterDescriptorSet(DescriptorSet* set);
  
  GraphicsPipeline*     UnregisterGraphicsPipeline(U64 id);
  ComputePipeline*      UnregisterComputePipeline(U64 id);
  FrameBuffer*          UnregisterFrameBuffer(U64 id);
  Texture*              UnregisterRenderTexture(U64 id);
  Sampler*              UnregisterSampler(U64 id);
  DescriptorSetLayout*  UnregisterDescriptorSetLayout(U64 id);
  DescriptorSet*        UnregisterDescriptorSet(U64 id);
};


Resources& gResources();
} // Recluse