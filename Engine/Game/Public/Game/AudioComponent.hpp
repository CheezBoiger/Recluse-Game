// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Component.hpp"
#include "Audio/Audio.hpp"
#include "Physics/RigidBody.hpp"

namespace Recluse {


// Audio emitter object. Used to emit sounds in the game world.
class AudioComponent : public Component {
  RCOMPONENT(AudioComponent)
public:

  void onInitialize(GameObject* owner) override;
  void onCleanUp() override;
  void onEnable() override { }
  void update() override;

  void setRigidBodyReference(RigidBody* pBody) { m_pRigidBodyRef = pBody; }
  void playSound(const std::string& soundPath, R32 volume);
  void pause();
  void setVolume(R32 volume);
  void isPaused();
  
private:

  RigidBody* m_pRigidBodyRef;
  U32 m_audioChannelId;
  U32 m_audioChannelGroupId;
};


// Listener component, used to listen to sounds in the game world.
class AudioListenerComponent : public Component {
  RCOMPONENT(AudioListenerComponent);
public:
  void onInitialize(GameObject* owner) override { }
  void onCleanUp() override { }
  void onEnable() override { }
  void update() override { }


private:
  U32 m_audioListenerId;
  U32 m_audioListenGroupId;
};
} // Recluse 