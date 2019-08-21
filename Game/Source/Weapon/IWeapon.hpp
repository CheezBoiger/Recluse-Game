// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Game/Engine.hpp"

using namespace Recluse;


class IActor;
 
enum WeaponType {
  WeaponType_NoType = (1 << 0),
  WeaponType_Melee = (1 << 1),
  WeaponType_Ranged = (1 << 2),
  WeaponType_Beam = (1 << 3),
  WeaponType_Area = (1 << 5),
};


enum BulletType {
  // No bullet type. Often used for melee weapons.
  BulletType_None = (1 << 0),

  // Standard bullet, normal rounds that deal normal damage and crit values.
  BulletType_Standard = (1 << 1),

  // Silver bullet. Deals additional damage to thick skin enemies.
  BulletType_Silver = (1 << 2),

  // Gold bullet. Deals Piercing damage to units with kevlar and steel.
  BulletType_Gold = (1 << 3),

  // Titanium bullet. Deals piercing damage to units with kevlar, steel, and any other reinforced
  // armor material. No effect on Thick skin enemies.
  BulletType_Titanium = (1 << 4),

  // Deals piercing damage to all materials. Includes additional random damage on demonic enemies.
  BulletType_Damascus = (1 << 5),
};


enum DamageEffects {
  // No effect.
  DamageEffects_None = (0),

  // Burning effect. Which deals damage over time and applies a percentage reduction of health regen.
  DamageEffects_Burning = (1 << 0),

  // Frozen effect. Deals damage over time and slows down enemies.
  DamageEffects_Frozen = (1 << 1),

  // Possessed effect. Allows bypassing shield and armor, while preventing energy regeneration.
  // Possessed enemies are also capable of instant death if health is dropped below a certain threshold.
  DamageEffects_Possessed = (1 << 2),

  // Stunned effects. Causes victim to be disabled for a duration.
  DamageEffects_Stunned = (1 << 3),

  // Dazed effect. Causes difficulty in movement on victim for a duration.
  DamageEffects_Dazed = (1 << 4),

  // Shocked effect. Boosts damage onto victim from enemies.
  DamageEffects_Shocked = (1 << 5),

  // Poisoned effect. Causes damage over time and a certain percentage of it will 
  // kill the victim instantly.
  DamageEffects_Poisoned = (1 << 6),

  // Slow effect. Slows target at a certain percentage.
  DamageEffects_Slowed = (1 << 7),

  // Bleeding effect. Causes target to lose health regen (may also negate health regen). No material can reduce its damage.
  DamageEffects_Bleeding = (1 << 8)
};


enum DamageType {
  // Normal damage. the higher the damage to armor ratio of the victim, the likely hood of it dealing Dazed or stunned
  // effects.
  DamageType_Normal = (1 << 0),

  // damage over time along with slow an frozen effect on victims. 
  DamageType_Chill = (1 << 1),

  // damage over time with burning effects on victims.
  DamageType_Incendiary = (1 << 2),

  // Blunt damage with likely cause for Daze and stun effects.
  DamageType_Blunt = (1 << 3),

  // Electrical damage may cause shocked damage effects.
  DamageType_Electrical = (1 << 4),

  // Demonic damage may cause possess effects.
  DamageType_Demonic = (1 << 5),

  // Slash damage may cause short stun effects.
  DamageType_Slash = (1 << 6),

  // Puncture damage has a likely chance of causing bleeding effects.
  DamageType_Puncture = (1 << 7),

  // Explosive damage deals dazed effects.
  DamageType_Explosive = (1 << 9),

  // Poison damage deals reduced energy regeneration along with damage to health.
  DamageType_Poison = (1 << 10)
};


// Armor materials define natural reduction to certain bullet types and damage types.
enum ArmorMaterial {
  // All damage dealt with effects.
  ArmorMaterial_None = (1 << 0),

  // Certain damage dealt depending on bullet type.

  // Kevlar reduces damage taken from standard, silver, and gold types. Reduces
  // damage taken from Normal, and slash damage.
  ArmorMaterial_Kevlar = (1 << 1),

  // Steel reduces damage taken from standard, silver, gold, and titanium. Reduces
  // damage taken from Normal, chill, incendiary, and puncture.
  ArmorMaterial_Steel = (1 << 2),

  // Tungstun reduces damage from standard, silver, gold, and titanium, along with 
  // mitigating daze and stun effects. Reduces damage taken from Incendiary, Blunt, and 
  ArmorMaterial_Tungstun = (1 << 3),

  // thick skin reduces damage from standard, and demonic. Completely removes stun, daze, and
  // shock. Reduces slash, blunt, puncture, and demonic damage.
  ArmorMaterial_ThickSkin = (1 << 4),

  // Demonic reduces all types, along with removing stun and daze.
  ArmorMaterial_Demonic = (1 << 5),

  // Shield allows damage absorption before dealing damage to health.
  ArmorMaterial_Shield = (1 << 6)
};


// Further reduction allowed in armor.
enum ArmorReduce {
  ArmorReduce_None = (1 << 0),
  ArmorReduce_Freeze = (1 << 1),
  ArmorReduce_Burn = (1 << 2),
  ArmorReduce_Explosive = (1 << 3),
  ArmorReduce_Demonic = (1 << 4)
};


typedef U32 WeaponTypeBits;
typedef U32 BulletTypeBits;
typedef U32 DamageEffectsBits;
typedef U32 ArmorReduceBits;
typedef U32 ArmorMaterialBits;
typedef U32 DamageTypeBits;


// Bullet interface.
class IBullet : public GameObject {
  R_GAME_OBJECT(IBullet)
public:
  virtual ~IBullet() = 0 { }

  BulletTypeBits GetBulletType() const { return m_bulletType; }

  virtual void update(R32 tick) override { }

private:
  BulletTypeBits  m_bulletType;
  DamageTypeBits  m_damageType;
  R32             m_damages[32];
  R32             m_critFactor;
  IActor*         m_pOwnerRef;
};


// Weapon interface.
class IWeapon : public GameObject {
  R_GAME_OBJECT(IWeapon)
public:
  virtual ~IWeapon() = 0 { }

  WeaponTypeBits GetWeaponType() const { return m_weaponType; }

  virtual void update(R32 tick) override { }

private:  
  std::string           m_name;
  WeaponTypeBits        m_weaponType;
  BulletTypeBits        m_bulletType;
  DamageTypeBits        m_damageType;
  R32                   m_minDamage[32];
  R32                   m_maxDamage[32];
  R32                   m_critFactor;
  IActor*               m_pOwnerRef;
}; 


// Armor interface.
class IArmor : public GameObject {
  R_GAME_OBJECT(IArmor)
public:
  virtual ~IArmor() = 0 { }

  ArmorMaterialBits GetArmorMaterial() const { return m_armorMaterial; }
  ArmorReduceBits GetArmorReductions() const { return m_armorReduce; }

  virtual void update(R32 tick) override { }

private:
  std::string         m_name;
  ArmorMaterialBits   m_armorMaterial;
  ArmorReduceBits     m_armorReduce;
  IActor*             m_pOwnerRef;
};



// Damage handling class, used to calculate damage done onto actors in the game world.
class WeaponDamageHandler {
public:
};