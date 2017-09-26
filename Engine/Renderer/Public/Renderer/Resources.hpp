// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class GraphicsPipeline;
class ComputePipeline;
class FrameBuffer;

typedef size_t resource_id_t;


// Resources handler, which maps out our pipelines and shaders and whatnot.
class Resources {
public:
  GraphicsPipeline*   GetGraphicsPipeline(resource_id_t uid);
  ComputePipeline*    GetComputePipeline(resource_id_t uid);
  FrameBuffer*        GetFrameBuffer(resource_id_t uid);

  resource_id_t       RegisterGraphicsPipeline(std::string name, GraphicsPipeline* pipeline);
  resource_id_t       RegisterComputePipeline(std::string name, ComputePipeline* pipeline);
  resource_id_t       RegisterFrameBuffer(std::string name, FrameBuffer* framebuffer);

  GraphicsPipeline*   UnregisterGraphicsPipeline(resource_id_t uid);
  ComputePipeline*    UnregisterComputePipeline(resource_id_t uid);
  FrameBuffer*        UnregisterFrameBuffer(resource_id_t uid);
};


Resources& gResources();
} // Recluse