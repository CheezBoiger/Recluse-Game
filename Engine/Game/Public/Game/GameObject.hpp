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

namespace Recluse {


typedef uuid64  game_uuid_t;
typedef uuid64  object_uuid_t;


// Always define this macro when inheriting GameObject (or any child objects that have
// GameObject as it's hierarchial parent.
#define R_GAME_OBJECT(cls)  \
  virtual object_uuid_t GetObjectId() const override { return cls :: GlobalId(); } \
  static game_uuid_t GlobalId() { return hash_bytes(#cls, strlen(#cls)); }

// Game Object, used for the game itself. These objects are the fundamental data type
// in our game, which hold important info regarding various data about transformation,
// physics, audio, animation, etc. GameObject only contains basic information about its
// children, therefore, you must inherit it to create and instantiate these components.
class GameObject : public ISerializable {

  GameObject(const GameObject&) = delete;
  GameObject& operator=(const GameObject&) = delete;

  static u64 sGameObjectCount;

public:

  static u64 NumGameObjectsCreated() { return sGameObjectCount; }
  static game_uuid_t GlobalId() { return hash_bytes("GameObject", strlen("GameObject")); }

  GameObject();
  ~GameObject();
  GameObject(GameObject&&);
  GameObject& operator=(GameObject&&);

  virtual void                        Serialize(IArchive& archive) override;
  virtual void                        Deserialize(IArchive& archive) override;
  void                                SetParent(GameObject* parent) { m_pParent = parent; }
  void                                SetName(std::string name) { m_name = name; }
  void                                SetTag(std::string tag) { m_tag = tag; }
  void                                AddChild(GameObject* child) { child->SetParent(this); m_children.push_back(child); }
  
  // Update the object. Can be overridable from inherited classes.
  virtual void                        Update(r32 tick) { }

  // Wakes up the game object in the scene.
  virtual void                        Awake() { }

  //  Performs necessary clean up on destroy.
  virtual void                        CleanUp() { }

  // Puts game object to sleep. This will cause engine to ignore updating this object.
  virtual void                        Sleep() { }

  virtual object_uuid_t               GetObjectId() const { return 0; }

  GameObject*                         GetParent() { return m_pParent; }
  GameObject*                         GetChild(std::string id);
  GameObject*                         GetChild(size_t idx) { if (m_children.size() > idx) return m_children[idx]; return nullptr; }

  Transform*                          GetTransform() { return &m_transform; }
  std::string                         GetName() const { return m_name; }
  std::string                         GetTag() const { return m_tag; }
  game_uuid_t                         GetId() const { return m_id; }
  size_t                              GetChildrenCount() const { return m_children.size(); }

  template<typename T>
  static T*  Cast(GameObject* obj) {
    T* n = nullptr;
    if (obj) {
      n = obj->GetObjectId() == T :: GlobalId() ? static_cast<T*>(obj) : nullptr;
    }
    return n;
  }

private:
  std::string                         m_name;
  std::string                         m_tag;

  // List of associated children.
  std::vector<GameObject*>            m_children;

  // Possible parent.
  GameObject*                         m_pParent;
  Transform                           m_transform;
  game_uuid_t                         m_id;
  object_uuid_t                       m_objTypeId;
  
  friend class GameObjectManager;
  friend class Component;
  friend class Scene;
  friend class Engine;
};
} // Recluse