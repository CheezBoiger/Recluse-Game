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

  void  onInitialize(GameObject* owner) override;
  void  onCleanUp() override;
  void  update() override;

  void  setIntensity(r32 intensity) override { m_NativeLight->_Intensity = intensity; }
  void  setColor(const Vector4& color) override { m_NativeLight->_Color = color; }
  void  setRange(r32 range) { m_NativeLight->_Range = range; }
  virtual void  onEnable() override { m_NativeLight->_Enable = enabled(); }
  
  void  setOffset(const Vector3& offset) { m_offset = offset; }
protected:
  void onDebug() override;

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
    , m_syncGameObject(true)
    , m_rotQuat(Quaternion::identity()) { }

  void  onInitialize(GameObject* owner) override;
  void  onCleanUp() override;
  void  update() override;

  void setRange(r32 range) { if (hasId()) m_NativeLight->_Range = range; }
  void setColor(const Vector4& color) override { if (hasId()) m_NativeLight->_Color = color; }
  void enableFixed(b32 enable) { m_fixed = enable; }
  void enableShadowing(b32 enable) { m_enableShadow = enable; }
  void enableSyncWithParent(b32 enable) { m_syncGameObject = enable; }

  void setOuterCutoff(r32 cutoff) { if (hasId()) m_NativeLight->_OuterCutOff = cutoff; }
  void setInnerCutoff(r32 cutoff) { if (hasId()) m_NativeLight->_InnerCutOff = cutoff; }
  void setIntensity(r32 intensity) override { if (hasId()) m_NativeLight->_Color.w = intensity; }
  virtual void onEnable() override { if (hasId()) m_NativeLight->_Enable = enabled(); }
  void setOffset(const Vector3& offset) { m_offset = offset; }
  void setRotationOffset(const Quaternion& rot) { m_rotQuat = rot; }

  Vector3 getPosition() const { return hasId() ? Vector3((r32*)&(m_NativeLight->_Position)) : Vector3(); }
  Vector3 getDirection() const { return hasId() ? Vector3((r32*)&(m_NativeLight->_Direction)) : Vector3(); }

  b32 hasId() const { return (b32)(m_NativeLight); }

  // Allows override of position and direction of spotlight, if SyncWithParent is disabled.
  // If SyncWithParent is enabled, these functions do not work, otherwise you need to manually
  // call these to update the transformation of the spotlight.
  void setDirection(const Vector3& dir) { if (hasId()) m_NativeLight->_Direction = Vector4(dir, 1.0f); }
  void setPosition(const Vector3& pos) { if (hasId()) m_NativeLight->_Position = Vector4(pos, 1.0f); }

private:
  Quaternion  m_rotQuat;
  SpotLight*  m_NativeLight;
  Vector3     m_offset;
  b32         m_fixed : 1,
              m_enableShadow : 1,
              m_syncGameObject : 1;
};
} // Recluse