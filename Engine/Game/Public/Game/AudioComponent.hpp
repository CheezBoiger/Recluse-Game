// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Component.hpp"
#include "Audio/Audio.hpp"
#include "Physics/RigidBody.hpp"

namespace Recluse {


class AudioComponent : public Component {
  RCOMPONENT(AudioComponent)
public:

  void onInitialize(GameObject* owner) override;
  void onCleanUp() override;
  void onEnable() override { }
  void update() override;

  void setRigidBodyReference(RigidBody* pBody) { m_pRigidBodyRef = pBody; }
  void playSound(const std::string& soundPath, r32 volume);
  void pause();
  void setVolume(r32 volume);
  void isPaused();
  
private:

  RigidBody* m_pRigidBodyRef;
  u32 m_audioChannelId;
  u32 m_audioChannelGroupId;
};
} // Recluse 