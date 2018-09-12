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


// Particle marks the current information used to process the position of the 
// particle during its lifetime. This is strictly gpu only information, stored into an SSBO 
// (Shader Storage Buffer Object.)
struct Particle {
  Vector4 _position;  // Position of the particle.
  Vector4 _velocity;
  Vector4 _color;
  r32     _angle;
  r32     _sz;        // size of the particle.
  r32     _weight;
  r32     _life;
};


struct ParticleLevel {
  // level determines at which point in a particle's life, will it 
  // trigger the specific texture index to be displayed, and for how long
  // until the next level of its life is reached. Max of 16 levels, which also means
  // a max of 16 levels allowed in a particle texture array.
  r32             _level[16];
  Texture2DArray* _texture;
};


enum ParticleUpdate {
  PARTICLE_BUFFER_UPDATE_BIT      = (1 << 0),
  PARTICLE_DESCRIPTOR_UPDATE_BIT  = (1 << 1)
};


using particle_update_bits = u32;


// Descriptor that defines a particle system, which allows cpu side interactions with
// how the system should calculate its particles.
struct ParticleSystem { 

  // Must initialize for compute pipeline as well!
  void                  Initialize(VulkanRHI* pRhi);
  void                  CleanUp(VulkanRHI* pRhi);
  void                  PushUpdate(particle_update_bits updateBits);
  void                  Update();

private:

  void                  ClearUpdateBits() { m_updateBits = 0x0; }
  DescriptorSet*        m_pDescriptorSet;

  // GPU based particle buffer. 
  Buffer*               m_particleBuffer;
  r32                   _lifeTimeScale;
  ParticleLevel         _particleLevel;
  particle_update_bits  m_updateBits;
};


// Particle engine handles all particles within the scene. This includes fire, smoke,
// liquid splashing, dust kicks, crazy explosions, etc.
// All calculations and rendering will be done on gpu side. 
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

  // Generate render commands for the given particles.
  void                GenerateParticleRenderCommands(CommandBuffer* cmdBuffer, ParticleSystem* descriptor);

  // Generate commands to compute particle positions and life. This will generate particle
  // computation commands for the given particle system.
  void                GenerateParticleComputeCommands(CommandBuffer* cmdBuffer, ParticleSystem* descriptor);

private:
  ComputePipeline*      m_pParticleCompute;
  GraphicsPipeline*     m_pParticleRender;
  DescriptorSetLayout*  m_pParticleDescriptorSetLayout;
};
} // Recluse