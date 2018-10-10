// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "ParticleSystemComponent.hpp"
#include "Renderer/Renderer.hpp"
#include "Core/Logging/Log.hpp"


namespace Recluse {

DEFINE_COMPONENT_MAP(ParticleSystemComponent);

void ParticleSystemComponent::OnInitialize(GameObject* owner)
{
  if (m_pParticleSystem) {
    return;
  }

  m_pParticleSystem = gRenderer().CreateParticleSystem();
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