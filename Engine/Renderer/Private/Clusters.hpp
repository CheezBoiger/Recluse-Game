// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "LightBVH.hpp"

namespace Recluse {


class ComputePipeline;


// Clusterer deals with computing clusters and assigning light indices to each cluster
// in the scene.
class Clusterer {
public:

private:
  ComputePipeline*  m_lightAssignmentPipe;
  ComputePipeline*  m_clusterGenPipe;
};
} // Recluse