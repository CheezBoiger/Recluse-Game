// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"

#include "Game/Component.hpp"

#define RBEHAVIOR(behav) RCOMPONENT(behav)
#define RSCRIPT(script) RBEHAVIOR(script)

namespace Recluse {


// Behavior is an abstract class that defines all aspects of game logic
// for our game object. This is what is used to define a script for the 
// game object.
struct IBehavior : public Component {
  B32      _Enabled; 

protected:
  // Mandatory that this update function is defined.
  virtual void  update() override { }

  virtual void awake() override { }
};


class IScript : public IBehavior {
public:
  virtual ~IScript() { }

  // Default start up of this behavior.
  virtual void  startUp() { }

  // Default shut down of this behavior.
  virtual void  shutDown() { }

  // Called when object is being initialized.
  virtual void  awake() override { }

  // Mandatory that this update function is defined.
  virtual void  update() override { }

  // when object is enabled, perform necessary actions.
  virtual void  onEnable() { }

  // When object is disabled, perform necessary actions.
  virtual void  OnDisable() { }

  // TODO(): To work on engine physics.
  virtual void OnCollisionEnter() { }
  virtual void OnCollisionStay() { }
  virtual void OnCollisionExit() { }

  // Enables this behavior on a game object.  
  void          setEnable() { }

  // Disables this behavior on a game object.
  void          Disable() { }
};
} // Recluse