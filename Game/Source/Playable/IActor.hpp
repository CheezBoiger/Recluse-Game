// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Game/GameObject.hpp"

using namespace Recluse;

namespace rs {



// Actor interface.
class IActor : public GameObject {
public:

  r32     GetMaxHealth() const { return m_maxHealth; }
  r32     GetCurrentHealth() const { return m_currHealth; }  

private:

  r32     m_maxHealth;
  r32     m_currHealth;
};
} // rs