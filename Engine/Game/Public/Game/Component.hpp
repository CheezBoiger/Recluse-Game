// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"

namespace Recluse {


class GameObject;


class Component : public ISerializable {
public:
  Component() 
    : mGameObjectOwner(nullptr) { }
  virtual ~Component() { }

  GameObject*   GetOwner() { return mGameObjectOwner; }

  virtual void  Update() { }

  virtual void  Serialize(IArchive& archive) { }
  virtual void  Deserialize(IArchive& archive) { }

protected:
  GameObject*   mGameObjectOwner;
};


class Transform : public Component {
public:
  // Front axis of the object in world space.
  Vector3       Front;

  // Up axis of the object in world space.
  Vector3       Up;

  // Right axis of the object in world space.
  Vector3       Right;
  
  // Local scale of this object. This should be relative to the parent.
  Vector3       LocalScale;

  // Local position, relative to the parent. If no parent is defined, this value is 
  // same as world space.
  Vector3       LocalPosition;
  
  // Position of the transform in world space.
  Vector3       Position;

  // Rotation of this transform in euler angles, this is in world coordinates.
  Vector3       EulerAngles;

  // Rotation of the transform relative to the parent.
  Quaternion    LocalRotation;

  // Rotation of the transform in world space.
  Quaternion    Rotation;
};
} // Recluse