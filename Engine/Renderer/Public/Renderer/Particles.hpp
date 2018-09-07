// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "RenderCmd.hpp"

#include "Core/Math/Vector4.hpp"
#include "Core/Math/Vector3.hpp"

#include <vector>

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
class CommandBuffer;
class Texture2DArray;


struct Particle {
  Vector4 _position;
  Vector4 _velocity;
  Vector4 _color;
  r32     _sz;
  r32     _angle;
  r32     _weight;
  r32     _life;
};


struct ParticleManager {
  Particle    _particles[2048];
  r32         _lifeTimeScale;

private:
  DescriptorSet*  _pDescriptorSet;
};


// Particle engine handles all particles within the scene. This includes fire, smoke,
// liquid splashing, dust kicks, crazy explosions, etc.
class ParticleEngine {
public:

  ParticleEngine() 
    : m_pParticleCompute(nullptr)
    , m_pParticleRender(nullptr)
    , m_pParticleDescriptorSetLayout(nullptr) { }

  ~ParticleEngine()
    { }

  void                Initialize(VulkanRHI* pRhi);
  void                CleanUp(VulkanRHI* pRhi);

  // Generate render commands for particles.
  void                GenerateParticleRenderCommands(CommandBuffer* cmdBuffer);

  // Generate commands to compute particle positions and life.
  void                GenerateParticleComputeCommands(CommandBuffer* cmdBuffer);

private:
  ComputePipeline*      m_pParticleCompute;
  GraphicsPipeline*     m_pParticleRender;
  DescriptorSetLayout*  m_pParticleDescriptorSetLayout;
};
} // Recluse