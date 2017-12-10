// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Camera.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Math/Common.hpp"


namespace Recluse {


Camera::Camera(Project type, r32 fov, r32 aspect, r32 zNear, r32 zFar, 
  Vector3 pos, Vector3 lookAt, Vector3 worldUp)
  : m_ProjType(type)
  , m_Fov(fov)
  , m_Aspect(aspect)
  , m_Position(pos)
  , m_LookAt(lookAt)
  , m_WorldUp(worldUp)
  , m_ZNear(zNear)
  , m_ZFar(zFar)
{
}


Matrix4 Camera::Projection()
{
  if (m_ProjType == ORTHO) { 
    R_DEBUG(rError, "Camera orthographic projection not implemented.\n");
  }

  return Matrix4::Perspective(m_Fov, m_Aspect, m_ZNear, m_ZFar);
}


Matrix4 Camera::View()
{
  // View matrix with standard look up.
  return Matrix4::LookAt(m_Position, m_LookAt, m_WorldUp);
}


void Camera::Update()
{
  m_Front = (m_LookAt - m_Position).Normalize();
  m_Right = m_Front.Cross(m_WorldUp).Normalize();
  m_Up = m_Right.Cross(m_Front).Normalize();
}


FirstPersonCamera::FirstPersonCamera(r32 fov, r32 aspect, r32 zNear, r32 zFar,
  Vector3 pos, Vector3 dir, Vector3 worldUp)
  : m_X_Sensitivity(0.2f)
  , m_Y_Sensitivity(0.2f)
  , m_Yaw(90.0f)
  , m_Pitch(0.0f)
  , m_ConstainedPitch(89.0f)
  , m_FirstLook(true)
  , m_LastX(0.0f)
  , m_LastY(0.0f)
  , m_Speed(5.0f)
  , m_Locked(false)
  , Camera(PERSPECTIVE, fov, aspect, zNear, zFar, pos, dir, worldUp)
{
}


Matrix4 FirstPersonCamera::View()
{
  return Matrix4::LookAt(m_Position, m_Front + m_Position, m_WorldUp);
}


void FirstPersonCamera::Move(Movement movement, r64 dt)
{
  // Delta time velocity.
  r32 Velocity = m_Speed * (r32)dt;

  switch (movement) {
    case FORWARD:
    {
      m_Position += m_Front * Velocity;
    } break;
    case BACK:
    {
      m_Position -= m_Front * Velocity;
    } break;
    case LEFT:
    {
      m_Position -= m_Right * Velocity;
    } break;
    case RIGHT:
    {
      m_Position += m_Right * Velocity;
    } break;
    case UP:
    {
    } break;
    case DOWN:
    {
    } break;
    default: break;
  }
}


void FirstPersonCamera::Update()
{
  m_Front.x = cosf(Radians(m_Yaw)) * cosf(Radians(m_Pitch));
  m_Front.y = sinf(Radians(m_Pitch));
  m_Front.z = sinf(Radians(m_Yaw)) * cosf(Radians(m_Pitch));
  m_Front = m_Front.Normalize();

  m_Right = m_Front.Cross(m_WorldUp).Normalize();
  m_Up = m_Right.Cross(m_Front).Normalize();
}


void FirstPersonCamera::Look(r64 x, r64 y)
{
  if (m_FirstLook) {
    m_LastX = (r32)x;
    m_LastY = (r32)y;
    m_FirstLook = false;
  }
  
  r32 xoffset = m_LastX - (r32)x;
  r32 yoffset =  m_LastY - (r32)y;
  m_LastX = (r32)x;
  m_LastY = (r32)y;

  xoffset *= m_X_Sensitivity;
  yoffset *= m_Y_Sensitivity;
  
  m_Yaw += xoffset;
  m_Pitch += yoffset;

  if (m_Pitch > m_ConstainedPitch) {
    m_Pitch = m_ConstainedPitch;
  }
  if (m_Pitch < -m_ConstainedPitch) {
    m_Pitch = -m_ConstainedPitch;
  }
}
} // Recluse