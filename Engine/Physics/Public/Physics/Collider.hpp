// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


class Actor;


class Collider {
public:
  virtual         ~Collider() { }


  virtual void    AssignToActor(Actor* actor) = 0;
  virtual void    CleanUp() = 0;
  virtual void    Initialize() = 0;
  
};
} // Recluse