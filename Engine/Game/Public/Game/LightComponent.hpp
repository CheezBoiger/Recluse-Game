// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Component.hpp"

#include <queue>

namespace Recluse {


// Basic Light component.
class LightComponent : public Component {
  RCOMPONENT(LightComponent);
  friend class Engine;
protected:
  static std::queue<u32>  sAvailableDirectionalLightIds;

  static void Initialize();
  static void CleanUp();

public:
  enum LightType {
    UNKNOWN_LIGHT,
    POINT_LIGHT,
    DIRECTION_LIGHT
  };

  LightComponent(LightType type = UNKNOWN_LIGHT)
    : m_Type(type)
    , m_Id(UINT_MAX) { }

  virtual ~LightComponent() { }

  virtual void              OnInitialize(GameObject* owner) override { }
  virtual void              OnCleanUp() override { }
  virtual void              Update() override { }

  u32                       GetId() const { return m_Id; }
  virtual void              SetIntensity(r32 intensity) { }
  virtual void              SetColor(const Vector4& color) { }

  virtual void              Serialize(IArchive& archive) override { }
  virtual void              Deserialize(IArchive& archive) override { }

protected:
  LightType                 m_Type;
  u32                       m_Id;
};
} // Recluse