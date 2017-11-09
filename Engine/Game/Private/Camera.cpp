// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Camera.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Math/Common.hpp"


namespace Recluse {


Camera::Camera(Project type, r32 fov, r32 aspect, r32 zNear, r32 zFar, 
  Vector3 pos, Vector3 lookAt, Vector3 worldUp)
  : mProjType(type)
  , mFov(fov)
  , mAspect(aspect)
  , mPosition(pos)
  , mLookAt(lookAt)
  , mWorldUp(worldUp)
  , mZNear(zNear)
  , mZFar(zFar)
{
}


Matrix4 Camera::Projection()
{
  if (mProjType == ORTHO) { 
    R_DEBUG(rError, "Camera orthographic projection not implemented.\n");
  }

  return Matrix4::Perspective(mFov, mAspect, mZNear, mZFar);
}


Matrix4 Camera::View()
{
  // View matrix with standard look up.
  return Matrix4::LookAt(mPosition, mLookAt, mWorldUp);
}


void Camera::Update()
{
  mFront = (mLookAt - mPosition).Normalize();
  mRight = mFront.Cross(mWorldUp).Normalize();
  mUp = mRight.Cross(mFront).Normalize();
}


FirstPersonCamera::FirstPersonCamera(r32 fov, r32 aspect, r32 zNear, r32 zFar,
  Vector3 pos, Vector3 dir, Vector3 worldUp)
  : xSensitivity(0.2f)
  , ySensitivity(0.2f)
  , mYaw(90.0f)
  , mPitch(0.0f)
  , mConstainedPitch(89.0f)
  , mFirstLook(true)
  , mLastX(0.0f)
  , mLastY(0.0f)
  , mSpeed(5.0f)
  , mLocked(false)
  , Camera(PERSPECTIVE, fov, aspect, zNear, zFar, pos, dir, worldUp)
{
}


Matrix4 FirstPersonCamera::View()
{
  return Matrix4::LookAt(mPosition, mFront + mPosition, mWorldUp);
}


void FirstPersonCamera::Move(Movement movement, r64 dt)
{
  // Delta time velocity.
  r32 velocity = mSpeed * (r32)dt;

  switch (movement) {
    case FORWARD:
    {
      mPosition += mFront * velocity;
    } break;
    case BACK:
    {
      mPosition -= mFront * velocity;
    } break;
    case LEFT:
    {
      mPosition -= mRight * velocity;
    } break;
    case RIGHT:
    {
      mPosition += mRight * velocity;
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
  mFront.x = cosf(Radians(mYaw)) * cosf(Radians(mPitch));
  mFront.y = sinf(Radians(mPitch));
  mFront.z = sinf(Radians(mYaw)) * cosf(Radians(mPitch));
  mFront = mFront.Normalize();

  mRight = mFront.Cross(mWorldUp).Normalize();
  mUp = mRight.Cross(mFront).Normalize();
}


void FirstPersonCamera::Look(r64 x, r64 y)
{
  if (mFirstLook) {
    mLastX = (r32)x;
    mLastY = (r32)y;
    mFirstLook = false;
  }
  
  r32 xoffset = mLastX - (r32)x;
  r32 yoffset =  mLastY - (r32)y;
  mLastX = (r32)x;
  mLastY = (r32)y;

  xoffset *= xSensitivity;
  yoffset *= ySensitivity;
  
  mYaw += xoffset;
  mPitch += yoffset;

  if (mPitch > mConstainedPitch) {
    mPitch = mConstainedPitch;
  }
  if (mPitch < -mConstainedPitch) {
    mPitch = -mConstainedPitch;
  }
}
} // Recluse