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
  Vector4 _position;  // _position of the particle.
  Vector4 _offsetPosition; // initial offset.
  Vector4 _velocity;
  Vector4 _initVelocity;
  Vector4 _acceleration;  // acceleration.
  Vector4 _color;
  Vector4 _info;          // x = angle, y = size, z = weight, w = life.
  Vector4 _camDist;
};


struct ParticleTrail {
  Vector4 _currWPos;     // last position.
  Vector4 _prevWPos;  // current position.
  Vector4 _nextWPos;     // next position.
  Vector4 _color;       // color.
  R32     _radius;
};


struct ParticleSystemConfig {
  Matrix4         _model;           // Model of the particle system source.
  Matrix4         _modelView;       // ModelView matrix.
  Vector4         _hasAtlas;          // 1.0 if system has an atlas.
  Vector4         _globalScale;     // global scale, this is a scaler!
  Vector4         _lightFactor;
  Vector4         _angleRate;         // rate of which to rotate.
  Vector4         _fadeIn;
  Vector4         _animScale;       // x = rate, y = maxOffset, z = offset
  R32             _fadeAt;
  R32             _fadeThreshold;
  R32             _angleThreshold; 
  R32             _rate;            // rate at which these particles are produced.
  R32             _lifeTimeScale;
  R32             _particleMaxAlive;     // Maximum particles that can be alive during the simulation.
  R32             _maxParticles;          // Maximum number of particles within this buffer.
  R32             _isWorldSpace;            // 0.0 if particles follow the model space, or 1.0, if particles are within world space.
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

using particle_update_bits = U32;


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

  typedef std::function<void(ParticleSystemConfig*, Particle*, U32)> ParticleUpdateFunct;
  typedef std::function<B32(const Particle&, const Particle&)> ParticleSortFunct;

  // Must initialize for compute pipeline as well!
  void                  initialize(VulkanRHI* pRhi, DescriptorSetLayout* particleLayout, U32 initialParticleCount);
  void                  cleanUp(VulkanRHI* pRhi);
  void                  pushUpdate(particle_update_bits updateBits) { m_updateBits |= updateBits; }
  void                  update(VulkanRHI* pRhi);

  // Get the current state of the particle buffer. Size is the current max particle count of this particle system.
  void                  getParticleState(Particle* output);
  
  void                  setSortFunct(ParticleSortFunct sortFunct) { m_sortFunct = sortFunct; }
  void                  setUpdateFunct(ParticleUpdateFunct updateFunct) { m_updateFunct = updateFunct; }

  void                  setParticleMaxCount(U32 maxCount) { 
    if (maxCount == _particleConfig._maxParticles) return; 
    _particleConfig._maxParticles = R32(maxCount);
    pushUpdate(PARTICLE_VERTEX_BUFFER_UPDATE_BIT);
  }

  void                  setParticleMaxLife(R32 maxLife);

  Texture2DArray*       _texture;
  TextureSampler*       _sampler;
  ParticleSystemConfig  _particleConfig;

  DescriptorSet*        getSet() const { return m_pDescriptorSet; }
  Buffer*               getParticleBuffer() const { return m_particleBuffer; }
  ParticleType          getParticleType() const { return m_particleType; }

private:
  void                  cpuBoundSort(std::vector<Particle>& particles);
  void                  cleanUpGpuBuffer(VulkanRHI* pRhi);
  void                  setUpGpuBuffer(VulkanRHI* pRhi);

  void                  updateDescriptor();
  void                  updateGpuParticles(VulkanRHI* pRhi);
  void                  clearUpdateBits() { m_updateBits = 0x0; }
  DescriptorSet*        m_pDescriptorSet;

  // GPU based particle buffer. 
  Buffer*               m_particleBuffer;

  // Particle Configuration buffer.
  Buffer*                   m_particleConfigBuffer;
  particle_update_bits      m_updateBits;
  ParticleUpdateFunct       m_updateFunct;
  ParticleSortFunct         m_sortFunct;
  ParticleType              m_particleType;
};


// Particle Trails.
class ParticleTrailSystem {
public:
  
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

  void                initialize(VulkanRHI* pRhi);
  void                cleanUp(VulkanRHI* pRhi);


  void                cleanUpPipeline(VulkanRHI* pRhi);
  void                initializePipeline(VulkanRHI* pRhi);

  // Generate render commands for the given particles.
  void                generateParticleRenderCommands(VulkanRHI* pRhi, CommandBuffer* cmdBuffer, GlobalDescriptor* global, CmdList<ParticleSystem*>& particleList, U32 frameIndex);

  // Generate commands to compute particle positions and life. This will generate particle
  // computation commands for the given particle system.
  void                generateParticleComputeCommands(VulkanRHI* pRhi, CommandBuffer* cmdBuffer, GlobalDescriptor* global, CmdList<ParticleSystem*>& particleList, U32 frameIndex);

  DescriptorSetLayout* getParticleSystemDescriptorLayout() { return m_pParticleDescriptorSetLayout; }
private:

  void                initializeRenderPass(VulkanRHI* pRhi);

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

  // render pass for the particle renderer.
  RenderPass*           m_pRenderPass;
};
} // Recluse