// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "RenderCmd.hpp"


namespace Recluse {


class GraphicsPipeline;
class ComputePipeline;
class StructureBuffer;
class Texture;
class DescriptorSetLayout;
class DescriptorSet;
class RenderPass;
class FrameBuffer;
class VulkanRHI;



// Particle engine handles all particles within the scene. This includes fire, smoke,
// liquid splashing, dust kicks, crazy explosions, etc.
class ParticleEngine {
public:

  ParticleEngine();
  ~ParticleEngine();

  void                Initialize(VulkanRHI* pRhi);
  void                CleanUp(VulkanRHI* pRhi);


};
} // Recluse