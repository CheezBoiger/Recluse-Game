// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Engine.hpp"
#include "Component.hpp"
#include "Core/Math/Vector4.hpp"


namespace Recluse {


struct ParticleSystem;
class Texture2DArray;

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
  void SetMaxParticleCount(u32 maxCount);
  void SetMaxAlive(r32 maxAlive);
  void SetLifetimeScale(r32 scale);
  void SetTextureArray(Texture2DArray* texture);
  void SetGlobalScale(r32 globalScale);

  void SetLevel(u32 idx, r32 at);

  void UseAtlas(b32 enable);

private:
  ParticleSystem* m_pParticleSystem;
};
} // Recluse