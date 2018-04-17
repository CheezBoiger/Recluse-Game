// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Camera.hpp"
#include "Component.hpp"


namespace Recluse {


class CameraComponent : public Component {
  RCOMPONENT(CameraComponent);
public:
  // Get the main camera being rendered through.
  static Camera&  GetMainCamera();

  void Update() override;
private:
  Camera*          m_pCamera;
};
} // Recluse