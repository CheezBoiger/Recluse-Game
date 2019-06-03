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
  static void   InitializeMeshDebug();
  static void   CleanUpMeshDebug();

  PointLightComponent()
    : LightComponent(LightComponent::POINT_LIGHT)
    , m_descriptor(nullptr) { }

  void  onInitialize(GameObject* owner) override;
  void  onCleanUp() override;
  void  update() override;

  void  setIntensity(r32 intensity) override { m_nativeLight._Intensity = intensity; }
  void  setColor(const Vector4& color) override { m_nativeLight._Color = color; }
  void  setRange(r32 range) { m_nativeLight._Range = range; }
  virtual void  onEnable() override { m_nativeLight._Enable = enabled(); }
  
  void  setOffset(const Vector3& offset) { m_offset = offset; }
protected:
  void onDebug() override;

private:
  MeshDescriptor* m_descriptor;
  PointLight m_nativeLight;  
  Vector3     m_offset;
};


class SpotLightComponent : public LightComponent {
  RCOMPONENT(SpotLightComponent);
public:

  SpotLightComponent()
    : LightComponent(LightComponent::SPOT_LIGHT)
    , m_fixed(false)
    , m_syncGameObject(true)
    , m_rotQuat(Quaternion::identity()) { }

  void  onInitialize(GameObject* owner) override;
  void  onCleanUp() override;
  void  update() override;

  void setRange(r32 range) { m_nativeLight._Range = range; }
  void setColor(const Vector4& color) override { m_nativeLight._Color = color; }
  void enableFixed(b32 enable) { m_fixed = enable; }
  void enableShadowing(b32 enable) { m_enableShadow = enable; }
  void enableSyncWithParent(b32 enable) { m_syncGameObject = enable; }

  void setOuterCutoff(r32 cutoff) { m_nativeLight._OuterCutOff = cutoff; }
  void setInnerCutoff(r32 cutoff) { m_nativeLight._InnerCutOff = cutoff; }
  void setIntensity(r32 intensity) override { m_nativeLight._Color.w = intensity; }
  virtual void onEnable() override { m_nativeLight._Enable = enabled(); }
  void setOffset(const Vector3& offset) { m_offset = offset; }
  void setRotationOffset(const Quaternion& rot) { m_rotQuat = rot; }

  Vector3 getPosition() const { return Vector3((r32*)&(m_nativeLight._Position)); }
  Vector3 getDirection() const { return Vector3((r32*)&(m_nativeLight._Direction)); }


  // Allows override of position and direction of spotlight, if SyncWithParent is disabled.
  // If SyncWithParent is enabled, these functions do not work, otherwise you need to manually
  // call these to update the transformation of the spotlight.
  void setDirection(const Vector3& dir) { m_nativeLight._Direction = Vector4(dir, 1.0f); }
  void setPosition(const Vector3& pos) { m_nativeLight._Position = Vector4(pos, 1.0f); }

private:
  Quaternion  m_rotQuat;
  SpotLight   m_nativeLight;
  Vector3     m_offset;
  b32         m_fixed : 1,
              m_enableShadow : 1,
              m_syncGameObject : 1,
              m_enable : 1;
};
} // Recluse