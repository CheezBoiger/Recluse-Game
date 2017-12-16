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

  void                SetPosition(Vector3 nPos) { m_Position = nPos; }
  void                SetWorldUp(Vector3 up) { m_WorldUp = up; }
  void                SetLookAt(Vector3 lookAt) { m_LookAt = lookAt; }
  void                SetAspect(r32 aspect) { m_Aspect = aspect; }
  void                SetFoV(r32 fov) { m_Fov = fov; }

  virtual void        Look(r64 x, r64 y) { }
  virtual void        Move(Movement move, r64 dt) { }
  Vector3             Position() const { return m_Position; }
  Vector3             LookDir() const { return m_LookAt; }
  Vector3             WorldUp() const { return m_WorldUp; }
  Quaternion          Rotation() const { return m_Rotation; }
  
  Ray                 GetDirectionRay() {  
    Ray camRay;
    camRay.Origin = m_Position;
    camRay.Direction = (m_LookAt - m_Position).Normalize();
    return camRay;
  }

  r32                 Aspect() const { return m_Aspect; }
  r32                 FoV() const { return m_Fov; }
  r32                 Near() const { return m_ZNear; }
  r32                 Far() const { return m_ZFar; }

  Vector3             Front() const { return m_Front; }
  Vector3             Right() const { return m_Right; }
  Vector3             Up() const { return m_Up; }

  r32                 Exposure() const { return m_Exposure; }
  r32                 Gamma() const { return m_Gamma; }
  b8                  Bloom() const { return m_Bloom; }
  b8                  Culling() const { return m_FrustumCull; }

  void                SetExposure(r32 exposure) { m_Exposure = exposure; }
  void                SetGamma(r32 gamma) { m_Gamma = gamma; }
  void                EnableBloom(b8 enable) { m_Bloom = enable; }
  void                EnableFrustumCull(b8 enable) { m_FrustumCull = enable; }

protected:
  Vector3             m_WorldUp;  

  // Camera coordinates.
  Vector3             m_Front;
  Vector3             m_Right;
  Vector3             m_Up;

  Vector3             m_Position;
  Vector3             m_LookAt;
  Quaternion          m_Rotation;
  Project             m_ProjType;
  r32                 m_Fov;
  r32                 m_Aspect;
  r32                 m_ZNear;
  r32                 m_ZFar;

  r32                 m_Gamma;
  r32                 m_Exposure;
  b8                  m_Bloom       : 1;
  b8                  m_FrustumCull  : 1;
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

  void                SetSpeed(r32 s) { m_Speed = s; }
  virtual void        Update() override;
  virtual void        Move(Movement movement, r64 dt) override;
  void                Look(r64 x, r64 y) override;
  void                SetSensitivityX(r32 x) { m_X_Sensitivity = x; }
  void                SetSensitivityY(r32 y) { m_Y_Sensitivity = y; }
  void                LockTarget(b8 enable) { m_Locked = enable; }
  r32                 Yaw() const { return m_Yaw; }
  r32                 Pitch() const { return m_Pitch; }
  r32                 SensitivityX() const { return m_X_Sensitivity; }
  r32                 SensitivityY() const { return m_Y_Sensitivity; }
  r32                 Speed() const { return m_Speed; }
  b8                  Locked() const { return m_Locked; }


protected:

  r32                 m_Speed;
  r32                 m_LastX;
  r32                 m_LastY;
  r32                 m_X_Sensitivity;
  r32                 m_Y_Sensitivity;
  r32                 m_Yaw;
  r32                 m_Pitch;  
  r32                 m_ConstainedPitch;
  b8                  m_FirstLook;
  b8                  m_Locked;
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