// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"

#include <algorithm>

namespace Recluse {

typedef u64 component_t;

#define RCOMPONENT(cls) friend class GameObject; \
    static component_t UUID() { return std::hash<tchar*>()( #cls ); } \
    static const tchar* GetName() { return #cls; } 

class GameObject;


// Component is the base class of all component modules within a game object.
// Components specify information that would otherwise be labeled as abstract,
// as they define information that may or may not be common in all game objects.
// These components may define information about Physics, audio, animation, etc.
class Component : public ISerializable {
  RCOMPONENT(Component)
public:
  virtual ~Component() { }

  GameObject*   GetOwner() { return mGameObjectOwner; }
  void          SetOwner(GameObject* newOwner) { mGameObjectOwner = newOwner; }

  virtual void  Serialize(IArchive& archive) { }
  virtual void  Deserialize(IArchive& archive) { }

protected:
  Component()
    : mGameObjectOwner(nullptr) { }

  GameObject*   mGameObjectOwner;
};


class Transform : public Component {
  RCOMPONENT(Transform)
public:

  Transform() { }

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

  void          Serialize(IArchive& archive) override { }
  void          Deserialize(IArchive& archive) override { }
};
} // Recluse