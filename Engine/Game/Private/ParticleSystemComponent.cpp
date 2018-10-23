// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "ParticleSystemComponent.hpp"
#include "Renderer/Renderer.hpp"
#include "Core/Logging/Log.hpp"

#include <random>
#include <algorithm>


namespace Recluse {

DEFINE_COMPONENT_MAP(ParticleSystemComponent);

void ParticleSystemComponent::OnInitialize(GameObject* owner)
{
  if (m_pParticleSystem) {
    return;
  }

  m_pParticleSystem = gRenderer().CreateParticleSystem();

  m_pParticleSystem->SetUpdateFunct([=] (ParticleSystemConfig* config, Particle* particles, u32 count) -> void {
    std::random_device dev;
    std::mt19937 twist(dev());
    std::uniform_real_distribution<r32> uni(-0.2f, 0.2f);
    std::uniform_real_distribution<r32> pos(-3.0f, 3.0f);
    r32 offset = config->_particleMaxAlive / config->_maxParticles;
    r32 life = 0.0f;
    for (size_t i = 0; i < count; ++i) {
      Particle& p = particles[i];
      p._position = Vector4(config->_model[3][0], config->_model[3][1], config->_model[3][2], 1.0f);
      p._offsetPosition = Vector4(pos(twist), 0.0f, pos(twist));
      p._color = Vector4(0.0f, 0.0f, 0.0f, 0.6f);
      p._velocity = Vector4(uni(twist), 0.0f/*uni(twist)*/, uni(twist), 0.0f);
      p._initVelocity = p._velocity;
      p._info.w = life;
      p._info.y = 0.5f;
      p._acceleration = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
      life += offset * config->_lifeTimeScale;
    }
  });

  m_pParticleSystem->SetSortFunct([=] (const Particle& p0, const Particle& p1) -> b32 {
    return p0._camDist.x > p1._camDist.x;
  });
  REGISTER_COMPONENT(ParticleSystemComponent, this);
}


void ParticleSystemComponent::OnCleanUp()
{
  if (!m_pParticleSystem) return;
  gRenderer().FreeParticleSystem(m_pParticleSystem);
  m_pParticleSystem = nullptr;
  UNREGISTER_COMPONENT(ParticleSystemComponent);
}


void ParticleSystemComponent::Update()
{
  if (!m_pParticleSystem) return;

  Transform* transform = GetTransform();
  ParticleSystemConfig* data = &m_pParticleSystem->_particleConfig;
  data->_model = transform->GetLocalToWorldMatrix();

  m_pParticleSystem->PushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
  if (m_shouldSort) {
    m_pParticleSystem->PushUpdate(PARTICLE_SORT_BUFFER_UPDATE_BIT);
  }

  gRenderer().PushParticleSystem(m_pParticleSystem);
}


void ParticleSystemComponent::SetMaxParticleCount(u32 maxCount)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->SetParticleMaxCount(maxCount);
}


void ParticleSystemComponent::EnableWorldSpace(b32 enable)
{
  if (!m_pParticleSystem) return;
  ParticleSystemConfig* data = &m_pParticleSystem->_particleConfig;
  if (enable) {
    data->_isWorldSpace = 1.0f;
  } else {
    data->_isWorldSpace = 0.0f;
  }
}


void ParticleSystemComponent::SetTextureArray(Texture2DArray* texture)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_texture = texture;
  m_pParticleSystem->_particleConfig._hasAtlas = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  m_pParticleSystem->PushUpdate(PARTICLE_DESCRIPTOR_UPDATE_BIT | PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::SetLevel(u32 idx, r32 at)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._level[idx] = at;
  m_pParticleSystem->PushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::UseAtlas(b32 enable)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._hasAtlas = Vector4(r32(enable), r32(enable), r32(enable), r32(enable));
  m_pParticleSystem->PushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::SetMaxAlive(r32 maxAlive)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._particleMaxAlive = maxAlive;
  m_pParticleSystem->PushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::SetLifetimeScale(r32 scale)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._lifeTimeScale = scale;
  m_pParticleSystem->PushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::SetGlobalScale(r32 scale)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._globalScale = scale;
  m_pParticleSystem->PushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::SetBrightnessFactor(r32 scale)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._lightFactor = scale;
  m_pParticleSystem->PushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}
} // Recluse