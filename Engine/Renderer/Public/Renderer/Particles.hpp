// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "RenderCmd.hpp"

#include "Core/Math/Vector4.hpp"
#include "Core/Math/Vector3.hpp"
#include "CmdList.hpp"

#include <vector>

namespace Recluse {


class GraphicsPipeline;
class ComputePipeline;
class StructureBuffer;
class Texture;
class GlobalDescriptor;
class DescriptorSetLayout;
class DescriptorSet;
class RenderPass;
class FrameBuffer;
class VulkanRHI;
class CommandBuffer;
class Texture2DArray;
class TextureSampler;


// Particle marks the current information used to process the position of the 
// particle during its lifetime. This is strictly gpu only information, stored into an SSBO 
// (Shader Storage Buffer Object.)
struct Particle {
  Vector4 _position;  // Position of the particle.
  Vector4 _offsetPosition; // initial offset.
  Vector4 _velocity;
  Vector4 _initVelocity;
  Vector4 _acceleration;  // acceleration.
  Vector4 _color;
  Vector4 _info;          // x = angle, y = size, z = weight, w = life.
  Vector4 _camDist;
};


struct ParticleTrail {
  Vector3 _lastPos;     // last position.
  Vector3 _currentPos;  // current position.
  Vector3 _nextPos;     // next position.
  Vector3 _info;        // x = size, y = weight, z = life.
};


struct ParticleSystemConfig {
  // level determines at which point in a particle's life, will it 
  // trigger the specific texture index to be displayed, and for how long
  // until the next level of its life is reached. Max of 16 levels, which also means
  // a max of 16 levels allowed in a particle texture array.
  Vector4         _level[16];
  Matrix4         _model;           // Model of the particle system source.
  Matrix4         _modelView;       // ModelView matrix.
  Vector4         _hasAtlas;          // 1.0 if system has an atlas.
  Vector4         _globalScale;     // global scale, this is a scaler!
  Vector4         _lightFactor;
  r32             _fadeAt;
  r32             _fadeThreshold;
  r32             _angleThreshold; 
  r32             _rate;            // rate at which these particles are produced.
  r32             _lifeTimeScale;
  r32             _particleMaxAlive;     // Maximum particles that can be alive during the simulation.
  r32             _maxParticles;          // Maximum number of particles within this buffer.
  r32             _isWorldSpace;            // 0.0 if particles follow the model space, or 1.0, if particles are within world space.
};


enum ParticleUpdate {
  PARTICLE_CONFIG_BUFFER_UPDATE_BIT      = (1 << 0),
  PARTICLE_DESCRIPTOR_UPDATE_BIT  = (1 << 1),
  PARTICLE_VERTEX_BUFFER_UPDATE_BIT = (1 << 2),
  PARTICLE_SORT_BUFFER_UPDATE_BIT = (1 << 3)
};

typedef enum eParticleType {
  PARTICLE_TYPE_POINT,
  PARTICLE_TYPE_TRAIL
} ParticleType;

using particle_update_bits = u32;


// Descriptor that defines a particle system, which allows cpu side interactions with
// how the system should calculate its particles.
struct ParticleSystem { 
  ParticleSystem() 
    : m_particleConfigBuffer(nullptr)
    , m_particleBuffer(nullptr)
    , m_pDescriptorSet(nullptr)
    , m_updateBits(0)
    , _texture(nullptr)
    , _sampler(nullptr)
    , m_particleType(PARTICLE_TYPE_POINT)
    , m_updateFunct(nullptr)
    , m_sortFunct(nullptr) { }

  typedef std::function<void(ParticleSystemConfig*, Particle*, u32)> ParticleUpdateFunct;
  typedef std::function<void(ParticleSystemConfig*, ParticleTrail*, u32)> ParticleTrailUpdateFunct;
  typedef std::function<b32(const Particle&, const Particle&)> ParticleSortFunct;

  // Must initialize for compute pipeline as well!
  void                  Initialize(VulkanRHI* pRhi, DescriptorSetLayout* particleLayout, u32 initialParticleCount);
  void                  CleanUp(VulkanRHI* pRhi);
  void                  PushUpdate(particle_update_bits updateBits) { m_updateBits |= updateBits; }
  void                  Update(VulkanRHI* pRhi);

  // Get the current state of the particle buffer. Size is the current max particle count of this particle system.
  void                  GetParticleState(Particle* output);
  void                  GetParticleTrailState(ParticleTrail* output);
  
  void                  SetSortFunct(ParticleSortFunct sortFunct) { m_sortFunct = sortFunct; }
  void                  SetUpdateFunct(ParticleUpdateFunct updateFunct) { m_updateFunct = updateFunct; }
  void                  SetTrailUpdateFunct(ParticleTrailUpdateFunct updateFunct) { m_updateTrailFunct = updateFunct; }

  void                  SetParticleMaxCount(u32 maxCount) { 
    if (maxCount == _particleConfig._maxParticles) return; 
    _particleConfig._maxParticles = r32(maxCount);
    PushUpdate(PARTICLE_VERTEX_BUFFER_UPDATE_BIT);
  }

  void                  SetParticleMaxLife(r32 maxLife);

  Texture2DArray*       _texture;
  TextureSampler*       _sampler;
  ParticleSystemConfig  _particleConfig;

  DescriptorSet*        GetSet() const { return m_pDescriptorSet; }
  Buffer*               GetParticleBuffer() const { return m_particleBuffer; }
  ParticleType          GetParticleType() const { return m_particleType; }

private:
  void                  CPUBoundSort(std::vector<Particle>& particles);
  void                  CleanUpGpuBuffer(VulkanRHI* pRhi);
  void                  SetUpGpuBuffer(VulkanRHI* pRhi);

  void                  UpdateDescriptor();
  void                  UpdateGpuParticles(VulkanRHI* pRhi);
  void                  ClearUpdateBits() { m_updateBits = 0x0; }
  DescriptorSet*        m_pDescriptorSet;

  // GPU based particle buffer. 
  Buffer*               m_particleBuffer;

  // Particle Configuration buffer.
  Buffer*                   m_particleConfigBuffer;
  particle_update_bits      m_updateBits;
  ParticleUpdateFunct       m_updateFunct;
  ParticleTrailUpdateFunct  m_updateTrailFunct;
  ParticleSortFunct         m_sortFunct;
  ParticleType              m_particleType;
};


// Particle engine handles all particles within the scene. This includes fire, smoke,
// liquid splashing, dust kicks, crazy explosions, etc.
// All calculations and rendering will be done on gpu side. 
class ParticleEngine {
public:

  ParticleEngine() 
    : m_pParticleCompute(nullptr)
    , m_pParticleRender(nullptr)
    , m_pParticleDescriptorSetLayout(nullptr)
    , m_pRenderPass(nullptr)
    , m_pFrameBuffer(nullptr) { }

  ~ParticleEngine();

  void                Initialize(VulkanRHI* pRhi);
  void                CleanUp(VulkanRHI* pRhi);


  void                CleanUpPipeline(VulkanRHI* pRhi);
  void                InitializePipeline(VulkanRHI* pRhi);

  // Generate render commands for the given particles.
  void                GenerateParticleRenderCommands(VulkanRHI* pRhi, CommandBuffer* cmdBuffer, GlobalDescriptor* global, CmdList<ParticleSystem*>& particleList);

  // Generate commands to compute particle positions and life. This will generate particle
  // computation commands for the given particle system.
  void                GenerateParticleComputeCommands(VulkanRHI* pRhi, CommandBuffer* cmdBuffer, GlobalDescriptor* global, CmdList<ParticleSystem*>& particleList);

  DescriptorSetLayout* GetParticleSystemDescriptorLayout() { return m_pParticleDescriptorSetLayout; }
private:

  void                InitializeRenderPass(VulkanRHI* pRhi);

  // Compute pipeline for particle calculations.
  ComputePipeline*      m_pParticleCompute;

  // Particle Renderer pipeline. This pipeline is instanced.
  GraphicsPipeline*     m_pParticleRender;

  // Particle trail Renderer pipeline. 
  GraphicsPipeline*     m_pParticleTrailRender;

  // 
  DescriptorSetLayout*  m_pParticleDescriptorSetLayout;

  // Framebuffer to the particle renderer.
  FrameBuffer*          m_pFrameBuffer;

  // Render pass for the particle renderer.
  RenderPass*           m_pRenderPass;
};
} // Recluse