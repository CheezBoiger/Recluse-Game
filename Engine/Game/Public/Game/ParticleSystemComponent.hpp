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
  virtual void    onInitialize(GameObject* owner) override;
  virtual void    onCleanUp() override;
  RCOMPONENT(ParticleSystemComponent);

public:
  ParticleSystemComponent()
    : m_pParticleSystem(nullptr)
    , m_shouldSort(false) { }

  void update() override;  
  
  void EnableWorldSpace(B32 enable);
  void SetMaxParticleCount(U32 maxCount);
  void SetMaxLife(R32 maxLife);
  void SetLifetimeScale(R32 scale);
  void SetTextureArray(Texture2DArray* texture);
  void SetGlobalScale(R32 globalScale);
  void SetBrightnessFactor(R32 scale);
  void SetFadeIn(R32 fadeIn);
  void SetFadeOut(R32 fadeOut);
  void SetRate(R32 rate);
  void EnableSorting(B32 enable) { m_shouldSort = enable; } 
  void SetAngleRate(R32 rate);
  void SetAnimationScale(R32 scale, R32 max = 16.0f, R32 offset = 0.0f);
  void SetAcceleration(const Vector3& acc) { m_acceleration = acc; }
  void setColor(const Vector4& color) { m_color = color; }

  void UseAtlas(B32 enable);

  B32 Sorting() const { return m_shouldSort; }

private:
  ParticleSystem* m_pParticleSystem;
  Vector3     m_acceleration;
  Vector4     m_color;
  B32 m_shouldSort;
};
} // Recluse