// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Plane.hpp"

namespace Recluse {


class Camera;
class AABB;

class CCamViewFrustum {
public:
  // camera that this view frustum is associated with.
  Camera*     m_Camera;

  // Planes of the camera. Ordered as so:
  // idx        plane
  // 0          left
  // 1          right
  // 2          top
  // 3          bottom
  // 4          near
  // 5          far
  Plane       m_Planes      [6];
  
  // TODO(): Update() function to calculate camera view frustum
  // which will be used to send to global material.
  void        Update();

  b8          Intersect(AABB* aabb);
  
};
} // Recluse