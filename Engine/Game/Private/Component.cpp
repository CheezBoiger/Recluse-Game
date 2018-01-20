// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Component.hpp"
#include "Engine.hpp"
#include "GameObject.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


void Transform::Update()
{
  LocalPosition = Position;
  LocalRotation = Rotation;
  EulerAngles = LocalRotation.ToEulerAngles();

  // TODO(): Update the transform's front, right, and up vectors
  // based on the local rotation.

}
} // Recluse