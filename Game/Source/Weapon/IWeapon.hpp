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
  WeaponType_Demonic = (1 << 4)
};


enum BulletType {
  BulletType_Standard = (1 << 0),
  BulletType_Silver = (1 << 1),
  BulletType_Gold = (1 << 2),
  BulletType_Titanium = (1 << 3),
  BulletType_Damascus = (1 << 4),
  BulletType_Demonic = (1 << 5)
};


enum WeaponAct {
  WeaponAct_None = (1 << 0),
  WeaponAct_Incendiary = (1 << 1),
  WeaponAct_Freeze = (1 << 2),
  WeaponAct_Possess = (1 << 3),
  WeaponAct_Stun = (1 << 4),
  WeaponAct_Daze = (1 << 5),
  WeaponAct_Shock = (1 << 6),
  WeaponAct_Explosive = (1 << 7)
};


typedef u32 WeaponTypeBits;
typedef u32 BulletTypeBits;
typedef u32 WeaponActBits;


class IBullet : public GameObject {
public:

private:
  
};


// Weapon interface.
class IWeapon : public GameObject {
public:


private:  
  WeaponTypeBits    m_weaponType;
  WeaponActBits     m_weaponActives;
}; 



// Damage handling class, used to calculate damage done onto actors in the game world.
class WeaponDamageHandler {
public:

};
}