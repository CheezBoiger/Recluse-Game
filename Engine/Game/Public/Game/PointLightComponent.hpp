// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Engine.hpp"
#include "Component.hpp"
#include "LightComponent.hpp"
#include "Renderer/LightDescriptor.hpp"


namespace Recluse {


class PointLightComponent : public Recluse::LightComponent {
  RCOMPONENT(PointLightComponent);
  
public:
  static std::queue<u32>  sAvailablePointLightIds;

  PointLightComponent()
    : LightComponent(LightComponent::POINT_LIGHT) { }

  void  OnInitialize(GameObject* owner) override;
  void  OnCleanUp() override;
  void  Update() override;

  void  SetIntensity(r32 intensity) override { m_NativeLight->_Intensity = intensity; }
  void  SetColor(const Vector4& color) override { m_NativeLight->_Color = color; }
  void  SetRange(r32 range) { m_NativeLight->_Range = range; }
  void  SetEnable(b8 enable) { m_NativeLight->_Enable = enable; }
  
private:
  PointLight* m_NativeLight;  
};
} // Recluse