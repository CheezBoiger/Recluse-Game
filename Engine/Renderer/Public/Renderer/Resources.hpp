// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class GraphicsPipeline;
class ComputePipeline;

typedef size_t resource_id_t;


// Resources handler, which maps out our pipelines and shaders and whatnot.
class Resources {
public:
  static Resources&   Global();

  GraphicsPipeline*   GetGraphicsPipeline(resource_id_t uid);
  ComputePipeline*    GetComputePipeline(resource_id_t uid);

  resource_id_t       RegisterGraphicsPipeline(std::string name, GraphicsPipeline* pipeline);
  resource_id_t       RegisterComputePipeline(std::string name, ComputePipeline* pipeline);
};
} // Recluse