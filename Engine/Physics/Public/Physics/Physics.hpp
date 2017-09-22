// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Time.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


class Actor;
class PhysX;

// Our physics engine.
class Physics : public EngineModule<Physics> {
public:
  Physics()
    : mDevice(nullptr) { }

  void                            OnStartUp() override;
  void                            OnShutDown() override;
  void                            UpdateState(r64 fixTime);

private:
  PhysX*                          mDevice;
};


// Global physics.
Physics& gPhysics();
} // Recluse