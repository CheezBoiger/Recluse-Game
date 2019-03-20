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
  static void globalInitialize();
  static void globalCleanUp();

public:
  enum LightType {
    UNKNOWN_LIGHT,
    POINT_LIGHT,
    DIRECTION_LIGHT,
    SPOT_LIGHT
  };

  LightComponent(LightType type = UNKNOWN_LIGHT)
    : m_Type(type)
    , m_Id(UINT_MAX)
    , m_debug(false) { }

  virtual ~LightComponent() { }

protected:
  virtual void              onInitialize(GameObject* owner) override { }
  virtual void              onCleanUp() override { }
  virtual void              update() override { }
public:

  u32                       getId() const { return m_Id; }
  virtual void              setIntensity(r32 intensity) { }
  virtual void              setColor(const Vector4& color) { }

  virtual void              serialize(IArchive& archive) override { }
  virtual void              deserialize(IArchive& archive) override { }

  b32                       isDebugging() { return m_debug; }
  
  void               enableDebug(b32 enable) { m_debug = enable; onDebug(); }

protected:

  virtual void onDebug() { }

  LightType                 m_Type;
  u32                       m_Id;
  b32                       m_debug;
};
} // Recluse