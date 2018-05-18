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

  // Global Clean up and initialization of light data, since this data structure keeps
  // track of light key references. Determines which light keys to assign.
  static void GlobalInitialize();
  static void GlobalCleanUp();

public:
  enum LightType {
    UNKNOWN_LIGHT,
    POINT_LIGHT,
    DIRECTION_LIGHT
  };

  LightComponent(LightType type = UNKNOWN_LIGHT)
    : m_Type(type)
    , m_Id(UINT_MAX)
    , m_debug(false) { }

  virtual ~LightComponent() { }

protected:
  virtual void              OnInitialize(GameObject* owner) override { }
  virtual void              OnCleanUp() override { }
  virtual void              Update() override { }
public:

  u32                       GetId() const { return m_Id; }
  virtual void              SetIntensity(r32 intensity) { }
  virtual void              SetColor(const Vector4& color) { }

  virtual void              Serialize(IArchive& archive) override { }
  virtual void              Deserialize(IArchive& archive) override { }

  b32                       Debugging() { return m_debug; }
  
 virtual void               Debug(b32 enable) { }

protected:
  LightType                 m_Type;
  u32                       m_Id;
  b32                       m_debug;
};
} // Recluse