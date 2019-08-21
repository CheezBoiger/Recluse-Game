// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Memory/SmartPointer.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/CharHash.hpp"

#include "Component.hpp"
#include "Scripts/Behavior.hpp"

#include <unordered_map>
#include <unordered_set>

namespace Recluse {


typedef UUID64  game_uuid_t;
typedef UUID64  object_uuid_t;
struct Collision;
class Scene;


// Always define this macro when inheriting GameObject (or any child objects that have
// GameObject as it's hierarchial parent.
#define R_GAME_OBJECT(cls)  \
  public: \
    static game_uuid_t globalId() { return hash_bytes(#cls, strlen(#cls)); } 

// Game Object, used for the game itself. These objects are the fundamental data type
// in our game, which hold important info regarding various data about transformation,
// physics, audio, animation, etc. GameObject only contains basic information about its
// children, therefore, you must inherit it to create and instantiate its components.
class GameObject : public ISerializable {

  GameObject(const GameObject&) = delete;
  GameObject& operator=(const GameObject&) = delete;

  static U64 sGameObjectCount;

public:

  static U64 numGameObjectsCreated() { return sGameObjectCount; }
  static game_uuid_t globalId() { return hash_bytes("GameObject", strlen("GameObject")); }

  GameObject();
  ~GameObject();
  GameObject(GameObject&&);
  GameObject& operator=(GameObject&&);

  virtual void                        serialize(IArchive& archive) override;
  virtual void                        deserialize(IArchive& archive) override;
  void                                setParent(GameObject* parent) { m_pParent = parent; }
  void                                setName(std::string name) { m_name = name; }
  void                                setTag(std::string tag) { m_tag = tag; }

  void addChild(GameObject* child) { 
    child->setParent(this);
    child->setSceneOwner(m_pScene);
    m_children.push_back(child);
  }
  
  // Update the object. Can be overridable from inherited classes.
  virtual void                        update(R32 tick) { }

protected:
  // Wakes up the game object in the scene. First time initialization is done with this call.
  virtual void                        onStartUp() { }

  //  Performs necessary clean up if Start() was called.
  virtual void                        onCleanUp() { }

  // On collision call. The can be overridden. 
  // other - Information of object colliding with this game object.
  virtual void                        onCollisionEnter(Collision* other) { }
  virtual void                        onCollisionExit(Collision* other) { }
  virtual void                        onCollisionStay(Collision* other) { }


public:
  // Puts game object to sleep. Called manually, and allows for certain components to be disabled if needed.
  virtual void                        sleep() { }

  GameObject*                         GetParent() { return m_pParent; }
  GameObject*                         getChild(std::string id);
  GameObject*                         getChild(size_t idx) { if (m_children.size() > idx) return m_children[idx]; return nullptr; }

  Transform*                          getTransform() { return &m_transform; }
  std::string                         getName() const { return m_name; }
  std::string                         getTag() const { return m_tag; }
  game_uuid_t                         getId() const { return m_id; }
  size_t                              getChildrenCount() const { return m_children.size(); }

  Scene*                              getSceneOwner() { return m_pScene; }

  void                                setSceneOwner(Scene* scene) { m_pScene = scene; }

  template<typename T>
  T*  castTo() {
    return dynamic_cast<T*>(this);
  }

  template<typename T>
  static T* cast(GameObject* obj) {
    if (!obj) return nullptr;
    return obj->castTo<T>();
  }

  B32                                 hasStarted() { return m_bStarted; }
  void                                cleanUp() { if (m_bStarted) { onCleanUp(); m_bStarted = false; } }
  void                                start() { if (!m_bStarted) { onStartUp(); m_bStarted = true; } }

private:

  std::string                         m_name;
  std::string                         m_tag;

  // List of associated children.
  std::vector<GameObject*>            m_children;

  // Possible parent.
  GameObject*                         m_pParent;
  Scene*                              m_pScene;
  Transform                           m_transform;
  game_uuid_t                         m_id;
  B32                                 m_bStarted;

  // Dispatch an event for collision. For Rigid Body use.
  void dispatchCollisionEnterEvent(Collision* collision) {
    onCollisionEnter(collision);
  }
  void dispatchCollisionExitEvent(Collision* collision) {
    onCollisionExit(collision);
  }
  void dispatchCollisionStayEvent(Collision* collision) {
    onCollisionStay(collision);
  }

  friend struct RigidBody;
  friend class Physics;
  friend class BulletPhysics;
  friend class GameObjectManager;
  friend class Component;
  friend class Engine;
};
} // Recluse