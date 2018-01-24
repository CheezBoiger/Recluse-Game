// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Memory/SmartPointer.hpp"
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
// Be aware, components can have only one ownership! Two game objects MAY NOT share
// the same component.
class Component : public ISerializable {
  RCOMPONENT(Component)
public:
  virtual ~Component() { }

  inline GameObject*   GetOwner() { return m_pGameObjectOwner; }

  virtual void  Serialize(IArchive& archive) { }
  virtual void  Deserialize(IArchive& archive) { }

protected:
  Component()
    : m_pGameObjectOwner(nullptr) { }

  template<typename T>
  static APtr<Component> Create() {
    return APtr<Component>(T());
  }

  // Perform early initialization of abstract component, then call OnInitialize() if any.
  void          Initialize(GameObject* owner) {
    m_pGameObjectOwner = owner;
    if (!m_pGameObjectOwner) {
      return;
    }

    OnInitialize(m_pGameObjectOwner);
  }

  // Mandatory that this update function is defined.
  virtual void  Update() { }

  // Optional fixed update, called from the physics engine updates.
  virtual void  FixedUpdate() { }

  virtual void  Awake() { }

  // Perform early clean up of abstract component, then call OnCleanUp() if any.
  void          CleanUp() {
    // Perform actions necessary for component clean up.
    OnCleanUp();
  }

  // Overrideable callbacks, in case component needs to initialize additional 
  // objects.
  virtual void  OnInitialize(GameObject* owner) { }
  virtual void  OnCleanUp() { }

private:
  GameObject*   m_pGameObjectOwner;
};


class Transform : public Component {
  RCOMPONENT(Transform)
public:

  Transform() 
    : m_Front(Vector3::FRONT)
    , m_Up(Vector3::UP)
    , m_Right(Vector3::RIGHT)
    , LocalScale(Vector3(1.0, 1.0f, 1.0f))
    , Rotation(Quaternion::AngleAxis(Radians(0.0f), Vector3::UP))
    , LocalRotation(Quaternion::AngleAxis(Radians(0.0f), Vector3::UP))
    , Scale(Vector3(1.0f, 1.0f, 1.0f))
    { }

  // Front axis of the object in world space.
  Vector3       Forward() const { return m_Front; };

  // Up axis of the object in world space.
  Vector3       Up() const { return m_Up; }

  // Right axis of the object in world space.
  Vector3       Right() const { return m_Right; };
  
  // Local scale of this object. This should be relative to the parent.
  Vector3       LocalScale;

  // Local position, relative to the parent. If no parent is defined, this value is 
  // same as world space.
  Vector3       LocalPosition;

  // Scale of the transform in world space.
  Vector3       Scale;
  
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

  Matrix4       GetLocalToWorldMatrix() const { return m_LocalToWorldMatrix; }
  Matrix4       GetWorldToLocalMatrix() const { return m_WorldToLocalMatrix; }

protected:
  // TODO():
  void          Update() override;
  void          FixedUpdate() override { }

  Matrix4       m_LocalToWorldMatrix;
  Matrix4       m_WorldToLocalMatrix;
  Vector3       m_Front;
  Vector3       m_Up;
  Vector3       m_Right;
};
} // Recluse