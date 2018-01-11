// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "Component.hpp"

#define RBEHAVIOR(behav) RCOMPONENT(behav)
#define RSCRIPT(script) RBEHAVIOR(script)

namespace Recluse {


// Behavior is an abstract class that defines all aspects of game logic
// for our game object. This is what is used to define a script for the 
// game object.
struct IBehavior  : public Component {
  b8      _Enabled; 
};


class IScript : public IBehavior {
public:
  virtual ~IScript() { }

  // Default start up of this behavior.
  virtual void  StartUp() { }

  // Default shut down of this behavior.
  virtual void  ShutDown() { }

  // Called when object is being initialized.
  virtual void  Awake() { }

  // Mandatory that this update function is defined.
  virtual void  Update() { }

  // Optional fixed update, called from the physics engine updates.
  virtual void  FixedUpdate() { }

  // when object is enabled, perform necessary actions.
  virtual void  OnEnable() { }

  // When object is disabled, perform necessary actions.
  virtual void  OnDisable() { }

  // Enables this behavior on a game object.  
  void          Enable();

  // Disables this behavior on a game object.
  void          Disable();
};
} // Recluse