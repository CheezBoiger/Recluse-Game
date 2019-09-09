// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "ParticleSystemComponent.hpp"
#include "Renderer/Renderer.hpp"
#include "Core/Logging/Log.hpp"

#include <random>
#include <algorithm>


namespace Recluse {

DEFINE_COMPONENT_MAP(ParticleSystemComponent);

void ParticleSystemComponent::onInitialize(GameObject* owner)
{
  if (m_pParticleSystem) {
    return;
  }

  m_pParticleSystem = gRenderer().createParticleSystem();

  m_pParticleSystem->setUpdateFunct([=] (ParticleSystemConfig* config, Particle* particles, U32 count) -> void {
    std::random_device dev;
    std::mt19937 twist(dev());
    std::uniform_real_distribution<R32> uni(-0.2f, 0.2f);
    std::uniform_real_distribution<R32> pos(-3.0f, 3.0f);
    R32 offset = config->_particleMaxAlive / config->_maxParticles;
    R32 life = 0.0f;
    for (size_t i = 0; i < count; ++i) {
      Particle& p = particles[i];
      p._position = Vector4(config->_model[3][0], config->_model[3][1], config->_model[3][2], 1.0f);
      p._offsetPosition = Vector4(pos(twist), 0.0f, pos(twist));
      p._color = m_color;
      p._velocity = Vector4(uni(twist), 0.0f/*uni(twist)*/, uni(twist), 0.0f);
      p._initVelocity = p._velocity;
      p._info.w = life;
      p._info.y = 0.5f;
      p._info.x = 180.0f;
      p._acceleration = Vector4(m_acceleration, 0.0f);
      life += offset * config->_lifeTimeScale;
    }
  });

  m_pParticleSystem->setSortFunct([=] (const Particle& p0, const Particle& p1) -> B32 {
    return p0._camDist.x > p1._camDist.x;
  });
  REGISTER_COMPONENT(ParticleSystemComponent, this);
}


void ParticleSystemComponent::onCleanUp()
{
  if (!m_pParticleSystem) return;
  gRenderer().freeParticleSystem(m_pParticleSystem);
  m_pParticleSystem = nullptr;
  UNREGISTER_COMPONENT(ParticleSystemComponent);
}


void ParticleSystemComponent::update()
{
  if (!enabled() || !m_pParticleSystem) return;

  Transform* transform = getTransform();
  ParticleSystemConfig* data = &m_pParticleSystem->_particleConfig;
  data->_model = transform->getLocalToWorldMatrix();

  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
  if (m_shouldSort) {
    m_pParticleSystem->pushUpdate(PARTICLE_SORT_BUFFER_UPDATE_BIT);
  }

  gRenderer().pushParticleSystem(m_pParticleSystem);
}


void ParticleSystemComponent::setMaxParticleCount(U32 maxCount)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->setParticleMaxCount(maxCount);
}


void ParticleSystemComponent::enableWorldSpace(B32 enable)
{
  if (!m_pParticleSystem) return;
  ParticleSystemConfig* data = &m_pParticleSystem->_particleConfig;
  if (enable) {
    data->_isWorldSpace = 1.0f;
  } else {
    data->_isWorldSpace = 0.0f;
  }
}


void ParticleSystemComponent::setTextureArray(Texture2DArray* texture)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_texture = texture;
  m_pParticleSystem->_particleConfig._hasAtlas = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  m_pParticleSystem->pushUpdate(PARTICLE_DESCRIPTOR_UPDATE_BIT | PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::useAtlas(B32 enable)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._hasAtlas = Vector4(R32(enable), R32(enable), R32(enable), R32(enable));
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setMaxLife(R32 maxLife)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._particleMaxAlive = maxLife;
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setLifetimeScale(R32 scale)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._lifeTimeScale = scale;
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setGlobalScale(R32 scale)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._globalScale = scale;
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setBrightnessFactor(R32 scale)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._lightFactor = scale;
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setFadeOut(R32 fadeOut)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._fadeAt = fadeOut;
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setFadeIn(R32 fadeIn)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._fadeIn = fadeIn;
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setAnimationScale(R32 scale, R32 max, R32 offset)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._animScale = Vector3(scale, max, offset);
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setAngleRate(R32 rate)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._angleRate = rate;
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}


void ParticleSystemComponent::setRate(R32 rate)
{
  if (!m_pParticleSystem) return;
  m_pParticleSystem->_particleConfig._rate = rate;
  m_pParticleSystem->pushUpdate(PARTICLE_CONFIG_BUFFER_UPDATE_BIT);
}
} // Recluse