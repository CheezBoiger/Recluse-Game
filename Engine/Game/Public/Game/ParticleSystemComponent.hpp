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
    : m_pParticleSystem(nullptr)
    , m_shouldSort(false) { }

  void Update() override;  
  
  void EnableWorldSpace(b32 enable);
  void SetMaxParticleCount(u32 maxCount);
  void SetMaxLife(r32 maxLife);
  void SetLifetimeScale(r32 scale);
  void SetTextureArray(Texture2DArray* texture);
  void SetGlobalScale(r32 globalScale);
  void SetBrightnessFactor(r32 scale);
  void SetFadeIn(r32 fadeIn);
  void SetFadeOut(r32 fadeOut);
  void SetRate(r32 rate);
  void EnableSorting(b32 enable) { m_shouldSort = enable; } 
  void SetAngleRate(r32 rate);

  void SetLevel(u32 idx, r32 at);

  void UseAtlas(b32 enable);

  b32 Sorting() const { return m_shouldSort; }

private:
  ParticleSystem* m_pParticleSystem;
  b32 m_shouldSort;
};
} // Recluse