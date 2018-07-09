// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Game/Engine.hpp"

using namespace Recluse;

namespace rs {
 
enum WeaponType {
  WeaponType_NoType = (1 << 0),
  WeaponType_Melee = (1 << 1),
  WeaponType_Ranged = (1 << 2),
  WeaponType_Beam = (1 << 3),
  WeaponType_Demonic = (1 << 4),
  WeaponType_Area = (1 << 5),
};


enum BulletType {
  BulletType_None = (1 << 0),
  BulletType_Standard = (1 << 1),
  BulletType_Silver = (1 << 2),
  BulletType_Gold = (1 << 3),
  BulletType_Titanium = (1 << 4),
  BulletType_Damascus = (1 << 5),
  BulletType_Demonic = (1 << 6)
};


enum WeaponAct {
  WeaponAct_None = (1 << 0),
  WeaponAct_Burn = (1 << 1),
  WeaponAct_Freeze = (1 << 2),
  WeaponAct_Possess = (1 << 3),
  WeaponAct_Stun = (1 << 4),
  WeaponAct_Daze = (1 << 5),
  WeaponAct_Shock = (1 << 6),
  WeaponAct_Explosive = (1 << 7),
  WeaponAct_Poison = (1 << 8)
};


enum ArmorMaterial {
  // All damage dealt with effects.
  ArmorMaterial_None = (1 << 0),

  // Certain damage dealt depending on bullet type.

  // Kevlar reduces damage taken from standard, silver, and gold.
  ArmorMaterial_Kevlar = (1 << 1),

  // Steel reduces damage taken from standard, silver, gold, and titanium.
  ArmorMaterial_Steel = (1 << 2),

  // Tungstun reduces damage from standard, silver, gold, and titanium, along with 
  // mitigating daze and stun effects.
  ArmorMaterial_Tungstun = (1 << 3),

  // thick skin reduces damage from standard, and demonic. Completely removes stun, daze, and
  // shock.
  ArmorMaterial_ThickSkin = (1 << 4),

  // Demonic reduces all effects, along with removing stun and daze.
  ArmorMaterial_Demonic = (1 << 5)
};


enum ArmorReduce {
  ArmorReduce_None = (1 << 0),
  ArmorReduce_Freeze = (1 << 1),
  ArmorReduce_Burn = (1 << 2),
  ArmorReduce_Explosive = (1 << 3),
  ArmorReduce_Demonic = (1 << 4)
};


typedef u32 WeaponTypeBits;
typedef u32 BulletTypeBits;
typedef u32 WeaponActBits;
typedef u32 ArmorReduceBits;
typedef u32 ArmorMaterialBits;


// Bullet interface.
class IBullet : public GameObject {
public:

private:
  BulletTypeBits m_bulltType;
};


// Weapon interface.
class IWeapon : public GameObject {
public:

  WeaponTypeBits GetWeaponType() const { return m_weaponType; }

  WeaponActBits GetWeaponActives() const { return m_weaponActives; }

private:  
  WeaponTypeBits    m_weaponType;
  WeaponActBits     m_weaponActives;
}; 


class IArmor : public GameObject {
public:
  
  ArmorMaterialBits GetArmorMaterial() const { return m_armorMaterial; }
  ArmorReduceBits GetArmorReductions() const { return m_armorReduce; }

private:
  ArmorMaterialBits       m_armorMaterial;
  ArmorReduceBits     m_armorReduce;
};



// Damage handling class, used to calculate damage done onto actors in the game world.
class WeaponDamageHandler {
public:
};
}