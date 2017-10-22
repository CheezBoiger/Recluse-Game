// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"


namespace Recluse {


// Virtual camera, which implements the pinhole theory.
// This is an inheritable class, so we can generate our FPS camera from,
// as well as our fly view camera.
class Camera {
public:
  Camera(Vector3 pos, Vector3 lookAt);


  virtual Matrix4     View();
  virtual Matrix4     Projection();

  void                SetPosition(Vector3 nPos) { mPosition = nPos; }
  void                SetWorldUp(Vector3 up) { mWorldUp = up; }

  Vector3             Position() const { return mPosition; }
  Vector3             LookPosition() const { return mLookAt; }
  Quaternion          Rotation() const { return mRotation; }

  r32                 Aspect() const { return mAspect; }
  r32                 FoV() const { return mFov; }

private:
  Vector3             mWorldUp;  

  // Camera coordinates.
  Vector3             mFront;
  Vector3             mRight;
  Vector3             mUp;

  Vector3     mPosition;
  Vector3     mLookAt;
  Quaternion  mRotation;

  r32         mFov;
  r32         mAspect;
};
} // Recluse