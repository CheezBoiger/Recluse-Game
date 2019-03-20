// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Quaternion.hpp"


namespace Recluse {


// Dual quaternion implemention, as defined by Unreal. Mainly used by animation.
// This is not my implementation!! Plz don't sue me Epic ;^;
class DualQuaternion {
public:
  Quaternion Real;
  Quaternion Dual;

  DualQuaternion(Quaternion r, Quaternion d)
    : Real(r), Dual(d) { }

  DualQuaternion operator+(const DualQuaternion& other) const;
  DualQuaternion operator*(const DualQuaternion& other) const;
  DualQuaternion operator*(const r32 scaler) const;
  
  DualQuaternion normalize() const;
};
} // Recluse