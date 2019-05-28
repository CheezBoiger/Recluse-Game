// Copyright (c) 2019 Recluse Project. All rights reserved.
#include "AudioComponent.hpp"



namespace Recluse {


DEFINE_COMPONENT_MAP(AudioComponent);


void AudioComponent::onInitialize(GameObject* owner)
{
  m_pRigidBodyRef = nullptr;
  REGISTER_COMPONENT(AudioComponent, this);
}


void AudioComponent::onCleanUp()
{
  UNREGISTER_COMPONENT(AudioComponent);
}


void AudioComponent::playSound(const std::string& soundPath, r32 volume)
{
  Transform* transform = getTransform();
  gAudio().loadSound(soundPath, true, true, false);
  m_audioChannelId = gAudio().initiateSound(soundPath, transform->_position, volume);
}


void AudioComponent::update()
{
  Vector3 vel = Vector3();
  if (m_pRigidBodyRef) vel = m_pRigidBodyRef->_velocity;
  gAudio().SetChannel3DPosition(m_audioChannelId, getTransform()->_position, vel);
}
} // Recluse