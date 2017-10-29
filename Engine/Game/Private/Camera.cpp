// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Camera.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"


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
    R_DEBUG("ERROR: camera orthographic projection not implemented.\n");
  }

  return Matrix4::Perspective(mFov, mAspect, mZNear, mZFar);
}


Matrix4 Camera::View()
{
  // View matrix with standard look up.
  return Matrix4::LookAt(mPosition, mLookAt, mWorldUp);
}
} // Recluse