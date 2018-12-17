// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Engine.hpp"
#include "Component.hpp"
#include "LightComponent.hpp"
#include "Renderer/LightDescriptor.hpp"


namespace Recluse {


class MeshDescriptor;


class PointLightComponent : public LightComponent {
  RCOMPONENT(PointLightComponent);
  
public:
  static std::queue<u32>  sAvailablePointLightIds;
  static void   InitializeMeshDebug();
  static void   CleanUpMeshDebug();

  PointLightComponent()
    : LightComponent(LightComponent::POINT_LIGHT)
    , m_descriptor(nullptr)
    , m_NativeLight(nullptr) { }

  void  OnInitialize(GameObject* owner) override;
  void  OnCleanUp() override;
  void  Update() override;

  void  SetIntensity(r32 intensity) override { m_NativeLight->_Intensity = intensity; }
  void  SetColor(const Vector4& color) override { m_NativeLight->_Color = color; }
  void  SetRange(r32 range) { m_NativeLight->_Range = range; }
  virtual void  OnEnable() override { m_NativeLight->_Enable = Enabled(); }
  
  void  SetOffset(const Vector3& offset) { m_offset = offset; }
protected:
  void OnDebug() override;

private:
  MeshDescriptor* m_descriptor;
  PointLight* m_NativeLight;  
  Vector3     m_offset;
};


class SpotLightComponent : public LightComponent {
  RCOMPONENT(SpotLightComponent);
public:
  static std::queue<u32> sAvailableSpotLightIds;

  SpotLightComponent()
    : LightComponent(LightComponent::SPOT_LIGHT)
    , m_NativeLight(nullptr)
    , m_fixed(false)
    , m_rotQuat(Quaternion::Identity()) { }

  void  OnInitialize(GameObject* owner) override;
  void  OnCleanUp() override;
  void  Update() override;

  void SetRange(r32 range) { m_NativeLight->_Range = range; }
  void SetColor(const Vector4& color) override { m_NativeLight->_Color = color; }
  void EnableFixed(b32 enable) { m_fixed = enable; }
  void EnableShadowing(b32 enable) { m_enableShadow = enable; }

  void SetOuterCutoff(r32 cutoff) { m_NativeLight->_OuterCutOff = cutoff; }
  void SetInnerCutoff(r32 cutoff) { m_NativeLight->_InnerCutOff = cutoff; }
  void SetIntensity(r32 intensity) override { m_NativeLight->_Color.w = intensity; }
  virtual void OnEnable() override { m_NativeLight->_Enable = Enabled(); }
  void SetOffset(const Vector3& offset) { m_offset = offset; }
  void SetRotationOffset(const Quaternion& rot) { m_rotQuat = rot; }

private:
  Quaternion  m_rotQuat;
  SpotLight*  m_NativeLight;
  Vector3     m_offset;
  b32         m_fixed : 1,
              m_enableShadow : 1;
};
} // Recluse