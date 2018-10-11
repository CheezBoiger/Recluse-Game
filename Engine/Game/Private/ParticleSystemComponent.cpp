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

  m_pParticleSystem->SetUpdateFunc([=] (ParticleSystemConfig* config, Particle* particles, u32 count) -> void {
    std::random_device dev;
    std::mt19937 twist(dev());
    std::uniform_real_distribution<r32> uni(-3.0f, 3.0f);
    r32 offset = config->_particleMaxAlive / config->_maxParticles;
    r32 life = 0.0f;
    for (size_t i = 0; i < count; ++i) {
      Particle& p = particles[i];
      p._position = Vector4(config->_model[3][0], config->_model[3][1], config->_model[3][2], 1.0f);
      p._color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      p._velocity = Vector4(uni(twist), uni(twist), uni(twist), 0.0f);
      p._initVelocity = p._velocity;
      p._life = life;
      p._sz = 0.1f;
      p._acceleration = Vector4(0.0f, -1.8f, 0.0f, 0.0f);
      life += offset * config->_lifeTimeScale;
    }
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
  gRenderer().PushParticleSystem(m_pParticleSystem);
}


void ParticleSystemComponent::SetMaxParticleCount(u32 maxCount)
{
  m_pParticleSystem->SetParticleMaxCount(maxCount);
}


void ParticleSystemComponent::EnableWorldSpace(b32 enable)
{
  ParticleSystemConfig* data = &m_pParticleSystem->_particleConfig;
  if (enable) {
    data->_isWorldSpace = 1.0f;
  } else {
    data->_isWorldSpace = 0.0f;
  }
}
} // Recluse