// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Matrix3.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Memory/SmartPointer.hpp"
#include <algorithm>
#include <unordered_map>

#include "Core/Utility/Profile.hpp"

namespace Recluse {

typedef u64 component_t;
class Transform;

#define RCOMPONENT(cls) protected: static std::unordered_map<uuid64, cls*> _k ## cls ## s; \
    friend class GameObject; \
    static uuid64 kUID; \
    static component_t getUUID() { return std::hash<tchar*>()( #cls ); } \
    static const tchar* getName() { return #cls; } \
    static uuid64 generateUID() { uuid64 uid = kUID++; return uid; } \
    public: static void updateComponents() { \
              for (auto& it : _k##cls##s) { \
                cls* comp = it.second; \
                if (!comp->enabled()) continue; \
                comp->update(); \
              } \
            } \
    private:

#define REGISTER_COMPONENT(cls, pComp) { \
          m_componentUID = generateUID(); \
          auto it = _k##cls##s.find(m_componentUID); \
          if (it == _k##cls##s.end()) { \
            _k##cls##s[m_componentUID] = pComp; \
          } \
       }

#define UNREGISTER_COMPONENT(cls) { \
          if (getOwner()) { \
            uuid64 uuid = getUID(); \
            auto it = _k##cls##s.find(uuid); \
            if (it != _k##cls##s.end()) { \
              _k##cls##s.erase(it); \
            } \
          } \
        }


#define DEFINE_COMPONENT_MAP(cls) std::unordered_map<uuid64, cls*> cls::_k##cls##s; \
                                  uuid64 cls::kUID = 0; 
                                
    

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

  inline GameObject*   getOwner() { return m_pGameObjectOwner; }

  virtual void  serialize(IArchive& archive) { }
  virtual void  deserialize(IArchive& archive) { }

protected:
  Component()
    : m_pGameObjectOwner(nullptr)
    , m_bEnabled(true) { }

  template<typename T>
  static APtr<Component> create() {
    return APtr<Component>(T());
  }

public:
  // Perform early initialization of abstract component, then call onInitialize() if any.
  void          initialize(GameObject* owner) {
    m_pGameObjectOwner = owner;
    if (!m_pGameObjectOwner) {
      return;
    }

    onInitialize(m_pGameObjectOwner);
    setEnable(true);
  }

  // Perform early clean up of abstract component, then call onCleanUp() if any.
  void          cleanUp() {
    // Perform actions necessary for component clean up.
    onCleanUp();
  }

  b32            enabled() const { return m_bEnabled; }

  void          setEnable(b32 enable) { m_bEnabled = enable; onEnable(); }

  Transform*    getTransform();

  uuid64        getUID() { return m_componentUID; }

protected:
  // Mandatory that this update function is defined.
  virtual void  update() { }

  virtual void  awake() { }

  // Overrideable callbacks, in case component needs to initialize additional 
  // objects.
  virtual void  onInitialize(GameObject* owner) { }
  virtual void  onCleanUp() { }
  virtual void  onEnable() { }

  uuid64        m_componentUID; 

private:
  GameObject*   m_pGameObjectOwner;
  // Is component enabled? If so, the managers responsible for updating it will 
  // proceed to do so.
  b32            m_bEnabled;
};


class Transform : public Component {
  RCOMPONENT(Transform)
public:

  Transform() 
    : m_front(Vector3::FRONT)
    , m_up(Vector3::UP)
    , m_right(Vector3::RIGHT)
    , _localScale(Vector3(1.0, 1.0f, 1.0f))
    , _rotation(Quaternion::angleAxis(Radians(0.0f), Vector3::UP))
    , _localRotation(Quaternion::angleAxis(Radians(0.0f), Vector3::UP))
    , _scale(Vector3(1.0f, 1.0f, 1.0f))
    { }

  Transform(const Transform& ob) {
    _localPosition = ob._localPosition;
    _position = ob._position;
    _rotation = ob._rotation;
    _localRotation = ob._localRotation;
    _scale = ob._scale;
    _localScale = ob._localScale;
    m_localToWorldMatrix = ob.m_localToWorldMatrix;
    m_worldToLocalMatrix = ob.m_worldToLocalMatrix;
    m_up = ob.m_up;
    m_front = ob.m_front;
    m_right = ob.m_right;
  }


  Transform& operator=(const Transform& ob) {
    _localPosition = ob._localPosition;
    _position = ob._position;
    _rotation = ob._rotation;
    _localRotation = ob._localRotation;
    _scale = ob._scale;
    _localScale = ob._localScale;
    m_localToWorldMatrix = ob.m_localToWorldMatrix;
    m_worldToLocalMatrix = ob.m_worldToLocalMatrix;
    m_up = ob.m_up;
    m_front = ob.m_front;
    m_right = ob.m_right;
    return (*this);
  }

  // front axis of the object in world space.
  Vector3       front() const { return m_front; };

  // up axis of the object in world space.
  Vector3       up() const { return m_up; }

  // right axis of the object in world space.
  Vector3       right() const { return m_right; };
  
  // Local scale of this object. This should be relative to the parent.
  Vector3       _localScale;

  // Local position, relative to the parent. If no parent is defined, this value is 
  // same as world space.
  Vector3       _localPosition;

  // _scale of the transform in world space.
  Vector3       _scale;
  
  // _position of the transform in world space.
  Vector3       _position;

  // Rotation of this transform in euler angles, this is in world coordinates.
  Vector3       _eulerAngles;

  // Rotation of the transform relative to the parent.
  Quaternion    _localRotation;

  // Rotation of the transform in world space.
  Quaternion    _rotation;

  void          serialize(IArchive& archive) override { }
  void          deserialize(IArchive& archive) override { }

  Matrix4       getLocalToWorldMatrix() const { return m_localToWorldMatrix; }
  Matrix4       getWorldToLocalMatrix() const { return m_worldToLocalMatrix; }

  // Update transform that will be used by renderer. Called during scene graph traversal.
  void          update() override;

protected:

  Matrix4       m_localToWorldMatrix;
  Matrix4       m_worldToLocalMatrix;
  Vector3       m_front;
  Vector3       m_up;
  Vector3       m_right;
};
} // Recluse