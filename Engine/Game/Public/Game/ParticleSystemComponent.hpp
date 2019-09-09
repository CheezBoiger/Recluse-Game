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
    , m_shouldSort(false)
    , m_shouldGPUSort(false) { }

  void update() override;  
  
  void enableWorldSpace(B32 enable);
  void setMaxParticleCount(U32 maxCount);
  void setMaxLife(R32 maxLife);
  void setLifetimeScale(R32 scale);
  void setTextureArray(Texture2DArray* texture);
  void setGlobalScale(R32 globalScale);
  void setBrightnessFactor(R32 scale);
  void setFadeIn(R32 fadeIn);
  void setFadeOut(R32 fadeOut);
  void setRate(R32 rate);
  void enableSorting(B32 enable) { m_shouldSort = enable; } 
  void setAngleRate(R32 rate);
  void setAnimationScale(R32 scale, R32 max = 16.0f, R32 offset = 0.0f);
  void setAcceleration(const Vector3& acc) { m_acceleration = acc; }
  void setColor(const Vector4& color) { m_color = color; }

  void useAtlas(B32 enable);

  B32 sorting() const { return m_shouldSort; }

private:
  ParticleSystem* m_pParticleSystem;
  Vector3     m_acceleration;
  Vector4     m_color;
  B32 m_shouldGPUSort;
  B32 m_shouldSort;
};
} // Recluse