// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Memory/SmartPointer.hpp"
#include "Core/Logging/Log.hpp"

#include "Component.hpp"
#include "Scripts/Behavior.hpp"

#include <unordered_map>

namespace Recluse {


typedef uuid64 game_uuid_t;

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

  GameObject();
  ~GameObject();
  GameObject(GameObject&&);
  GameObject& operator=(GameObject&&);

  virtual void                        Serialize(IArchive& archive) override;
  virtual void                        Deserialize(IArchive& archive) override;
  void                                SetParent(GameObject* parent) { m_pParent = parent; }
  void                                SetName(std::string name) { m_name = name; }
  void                                AddChild(GameObject* child) { child->SetParent(this); m_children.push_back(child); }
  
  // Update the object. Can be overridable from inherited classes.
  virtual void                        Update(r32 tick) { }

  // Wakes up the game object in the scene.
  virtual void                        Awake() { }

  //  Performs necessary clean up on destroy.
  virtual void                        CleanUp() { }

  // Puts game object to sleep. This will cause engine to ignore updating this object.
  virtual void                        Sleep() { }

  GameObject*                         GetParent() { return m_pParent; }
  GameObject*                         GetChild(std::string id);
  GameObject*                         GetChild(size_t idx) { if (m_children.size() > idx) return m_children[idx]; return nullptr; }

  Transform*                          GetTransform() { return &m_transform; }
  std::string                         GetName() const { return m_name; }
  game_uuid_t                         GetId() const { return m_id; }
  size_t                              GetChildrenCount() const { return m_children.size(); }

private:
  std::string                         m_name;

  // List of associated children.
  std::vector<GameObject*>            m_children;

  // Possible parent.
  GameObject*                         m_pParent;
  Transform                           m_transform;
  game_uuid_t                         m_id;

  friend class GameObjectManager;
  friend class Component;
  friend class Scene;
  friend class Engine;
};
} // Recluse