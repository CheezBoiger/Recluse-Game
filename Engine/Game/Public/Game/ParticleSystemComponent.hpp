// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Engine.hpp"
#include "Component.hpp"


namespace Recluse {


struct ParticleSystem;


class ParticleSystemComponent : public Component {
protected:
  virtual void    OnInitialize(GameObject* owner) override;
  virtual void    OnCleanUp() override;
  RCOMPONENT(ParticleSystemComponent);

public:
  ParticleSystemComponent()
    : m_pParticleSystem(nullptr) { }

  void Update() override;  
  
  void EnableWorldSpace(b32 enable);

private:
  ParticleSystem* m_pParticleSystem;
};
} // Recluse