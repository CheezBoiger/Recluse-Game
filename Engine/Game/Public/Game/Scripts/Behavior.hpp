// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "Component.hpp"

namespace Recluse {


// Behavior is an abstract class that defines all aspects of game logic
// for our game object. This is what is used to define a script for the 
// game object.
class CBehavior : public Component {
public:

  // Default start up of this behavior.
  virtual void StartUp() { }
  
  // Default shut down of this behavior.
  virtual void ShutDown() { }
  
  // Called when object is being initialized.
  virtual void Initialize() { }

  // Mandatory that this update function is defined.
  virtual void Update() = 0;

  // Optional fixed update, called from the physics engine updates.
  virtual void FixedUpdate() { }

  // when object is enabled, perform necessary actions.
  virtual void OnEnable() { }

  // When object is disabled, perform necessary actions.
  virtual void OnDisable() { }  
};
} // Recluse