// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Game/GameObject.hpp"
#include "Game/Engine.hpp"
#include "../Weapon/IWeapon.hpp"

using namespace Recluse;

namespace rs {


class IArmor;
class IWeapon;


// Actor interface.
class IActor : public GameObject {
  R_GAME_OBJECT(IActor)
public:
  enum Group {
      Group_None,
      Group_Ally,
      Group_Enemy
  };

  virtual ~IActor() = 0 { }

  r32     GetMaxHealth() const { return m_maxHealth; }
  r32     GetCurrentHealth() const { return m_currHealth; }  

  virtual void Update(r32 tick) override { }
  virtual void OnStart() override { }
  virtual void OnCleanUp() override { }
  
private:
  // Name of this actor.
  std::string m_name;

  // Current damage effects on this actor.
  DamageEffectsBits m_damageEffects;

  // group that this actor is associated with.
  Group       m_group;
  r32         m_maxHealth;
  r32         m_currHealth;
  r32         m_maxHealthRegen;
  r32         m_currHealthRegen;

  // Physics component of this actor.
  PhysicsComponent m_physicsComponent;
};
} // rs