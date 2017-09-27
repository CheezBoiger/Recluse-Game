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

typedef size_t resource_id_t;


// Resources handler, which maps out our pipelines and shaders and whatnot.
class Resources {
  static resource_id_t idCount;
  resource_id_t ObtainResourceId();

public:
  GraphicsPipeline*     GetGraphicsPipeline(resource_id_t uid);
  ComputePipeline*      GetComputePipeline(resource_id_t uid);
  FrameBuffer*          GetFrameBuffer(resource_id_t uid);
  Texture*              GetRenderTexture(resource_id_t uid);
  Sampler*              GetSampler(resource_id_t uid);
  DescriptorSetLayout*  GetDescriptorSetLayout(resource_id_t uid);

  resource_id_t         RegisterGraphicsPipeline(GraphicsPipeline* pipeline);
  resource_id_t         RegisterComputePipeline(ComputePipeline* pipeline);
  resource_id_t         RegisterFrameBuffer(FrameBuffer* framebuffer);
  resource_id_t         RegisterRenderTexture(Texture* texture);
  resource_id_t         RegisterSampler(Sampler* sampler);
  resource_id_t         RegisterDescriptorSetLayout(DescriptorSetLayout* layout);
  
  GraphicsPipeline*     UnregisterGraphicsPipeline(resource_id_t uid);
  ComputePipeline*      UnregisterComputePipeline(resource_id_t uid);
  FrameBuffer*          UnregisterFrameBuffer(resource_id_t uid);
  Texture*              UnregisterRenderTexture(resource_id_t uid);
  Sampler*              UnregisterSampler(resource_id_t uid);
  DescriptorSetLayout*  UnregisterDescriptorSetLayout(resource_id_t uid);
};


Resources& gResources();
} // Recluse