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
  enum Project {
    ORTHO,
    PERSPECTIVE
  };

  Camera(Project type, r32 fov, r32 aspect, r32 zNear, r32 zFar, 
    Vector3 pos, Vector3 lookAt, Vector3 worldUp);

  virtual Matrix4     View();
  virtual Matrix4     Projection();

  void                SetPosition(Vector3 nPos) { mPosition = nPos; }
  void                SetWorldUp(Vector3 up) { mWorldUp = up; }
  void                SetLookAt(Vector3 lookAt) { mLookAt = lookAt; }
  void                SetAspect(r32 aspect) { mAspect = aspect; }
  void                SetFoV(r32 fov) { mFov = fov; }

  Vector3             Position() const { return mPosition; }
  Vector3             LookPosition() const { return mLookAt; }
  Quaternion          Rotation() const { return mRotation; }

  r32                 Aspect() const { return mAspect; }
  r32                 FoV() const { return mFov; }

protected:
  Vector3             mWorldUp;  

  // Camera coordinates.
  Vector3             mFront;
  Vector3             mRight;
  Vector3             mUp;

  Vector3             mPosition;
  Vector3             mLookAt;
  Quaternion          mRotation;
  Project             mProjType;
  r32                 mFov;
  r32                 mAspect;
  r32                 mZNear;
  r32                 mZFar;
};


// First person camera.
class FirstPersonCamera : public Camera {
public:
  static r32      MAX_YAW;

  FirstPersonCamera();

  virtual Matrix4 View() override;
  virtual Matrix4 Projection() override;

  void            Look(r64 x, r64 y);
  r32             Yaw() const { return mYaw; }
  r32             Pitch() const { return mPitch; }


protected:

  r32             mYaw;
  r32             mPitch;  
};
} // Recluse