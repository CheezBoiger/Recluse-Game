// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Game/GameObject.hpp"
#include "Game/Engine.hpp"
#include "Game/AudioComponent.hpp"
#include "../Weapon/IWeapon.hpp"

using namespace Recluse;


class IArmor;
class IWeapon;
class IItem;



struct IHUDState {
  r32         _maxState;
  r32         _currState;
  r32         _maxRegen;
  r32         _currRegen;
};

// Actor interface that defines a living being in the world.
class IActor : public GameObject {
  R_GAME_OBJECT(IActor)
public:
  enum Activity {
    Activity_None = 0,
    Activity_Moving = (1<<0),
    Activity_Jumping = (1<<1),
    Activity_DoubleJumping = (1<<2),
    Activity_Crouching = (1<<3),
    Activity_Sliding = (1<<4),
    Activity_Prone = (1<<5)
  };

  enum Group {
      Group_None = 0,
      Group_Ally = (1<<0),
      Group_Enemy = (1<<1),
      Group_Owner = (1<<2),
      Group_ = (1<<3)
  };

  typedef u32 ActivityBits;

  virtual ~IActor() = 0 { }

  virtual void update(r32 tick) override { }
  virtual void onStartUp() override { }
  virtual void onCleanUp() override { }


  DamageEffectsBits GetActiveDamageEffects() const { return m_damageEffects; }
  ActivityBits    GetActivities() const { return m_activity; }

  IArmor*         GetActiveArmor() { return m_pArmor; }
  IWeapon*        GetActiveWeapon() { return m_pWeapon; }
  
protected:
  // Name of this actor.
  std::string m_name;

  // Current damage effects on this actor.
  DamageEffectsBits m_damageEffects;
  ActivityBits      m_activity;

  // group that this actor is associated with.
  Group       m_group;

  // current movement speed of the actor.
  r32         m_currentMoveSpeed;

  // Maximum movement speed of the actor.
  r32         m_maxMoveSpeed;

  // Jump strength.
  r32         m_jumpStrength;

  r32         m_sprintFactor;

  // Health.
  IHUDState   m_health;

  // Energy.
  IHUDState   m_energy;

  // Armor currently active.
  IArmor*     m_pArmor;

  // Weapon currently active.
  IWeapon*    m_pWeapon;
    
  // Physics component of this actor.
  PhysicsComponent m_physicsComponent;
};