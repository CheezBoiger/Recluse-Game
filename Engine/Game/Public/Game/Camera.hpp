// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Ray.hpp"

#include "Core/Utility/Vector.hpp"


namespace Recluse {


class GameObject;

// Virtual camera, which implements the pinhole theory.
// This is an inheritable class, so we can generate our FPS camera from,
// as well as our fly view camera.
class Camera {
public:
  // Movement for the camera.
  enum Movement {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    FORWARD,
    BACK
  };

  enum Project {
    ORTHO,
    PERSPECTIVE
  };

  Camera(Project type, r32 fov, r32 aspect, r32 zNear, r32 zFar, 
    Vector3 pos, Vector3 lookAt, Vector3 worldUp);

  virtual Matrix4     View();
  virtual Matrix4     Projection();

  // Update camera's coordinate view space.
  virtual void        Update();

  void                SetPosition(Vector3 nPos) { mPosition = nPos; }
  void                SetWorldUp(Vector3 up) { mWorldUp = up; }
  void                SetLookAt(Vector3 lookAt) { mLookAt = lookAt; }
  void                SetAspect(r32 aspect) { mAspect = aspect; }
  void                SetFoV(r32 fov) { mFov = fov; }

  virtual void        Look(r64 x, r64 y) { }
  virtual void        Move(Movement move, r64 dt) { }
  Vector3             Position() const { return mPosition; }
  Vector3             LookDir() const { return mLookAt; }
  Vector3             WorldUp() const { return mWorldUp; }
  Quaternion          Rotation() const { return mRotation; }
  
  Ray                 GetDirectionRay() {  
    Ray camRay;
    camRay.Origin = mPosition;
    camRay.Direction = (mLookAt - mPosition).Normalize();
    return camRay;
  }

  r32                 Aspect() const { return mAspect; }
  r32                 FoV() const { return mFov; }
  r32                 Near() const { return mZNear; }
  r32                 Far() const { return mZFar; }

  Vector3             Front() const { return mFront; }
  Vector3             Right() const { return mRight; }
  Vector3             Up() const { return mUp; }

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


// First person camera. This camera affects the movement of how the 
// player sees the world around them. These will likely be deprecated 
// as a result of our game object being constructed.
class FirstPersonCamera : public Camera {
public:
  static r32          MAX_YAW;

  FirstPersonCamera(r32 fov, r32 aspect, r32 zNear, r32 zFar, 
    Vector3 pos, Vector3 dir, Vector3 worldUp);

  virtual Matrix4     View() override;

  void                SetSpeed(r32 s) { mSpeed = s; }
  virtual void        Update() override;
  virtual void        Move(Movement movement, r64 dt) override;
  void                Look(r64 x, r64 y) override;
  void                SetSensitivityX(r32 x) { xSensitivity = x; }
  void                SetSensitivityY(r32 y) { ySensitivity = y; }
  void                LockTarget(b8 enable) { mLocked = enable; }
  r32                 Yaw() const { return mYaw; }
  r32                 Pitch() const { return mPitch; }
  r32                 SensitivityX() const { return xSensitivity; }
  r32                 SensitivityY() const { return ySensitivity; }
  r32                 Speed() const { return mSpeed; }
  b8                  Locked() const { return mLocked; }


protected:

  r32                 mSpeed;
  r32                 mLastX;
  r32                 mLastY;
  r32                 xSensitivity;
  r32                 ySensitivity;
  r32                 mYaw;
  r32                 mPitch;  
  r32                 mConstainedPitch;
  b8                  mFirstLook;
  b8                  mLocked;
};


// Fly view camera, for other cool effects such as cutscenes and crap.
class FlyViewCamera : public FirstPersonCamera {
public:
  FlyViewCamera(r32 fov, r32 aspect, r32 zNear, r32 zFar, 
    Vector3 pos, Vector3 dir, Vector3 worldUp, r32 speed = 10.0f);
  virtual void        Move(Movement movement, r64 dt) override;

protected:
  
};


// ArcBall style camera. Allows you to rotate camera around a certain target point.
class ArcBallCamera final : public Camera {
public:

};
} // Recluse