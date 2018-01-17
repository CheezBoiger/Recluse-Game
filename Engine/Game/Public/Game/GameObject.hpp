// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Memory/SmartPointer.hpp"

#include "Component.hpp"
#include "Scripts/Behavior.hpp"

#include <unordered_map>

namespace Recluse {


typedef u64 game_uuid_t;

// Game Object, used for the game itself. These objects are the fundamental data type
// in our game, which hold important info regarding various data about transformation,
// physics, audio, animation, etc.
class GameObject : public ISerializable {

  GameObject(const GameObject&) = delete;
  GameObject& operator=(const GameObject&) = delete;

public:

  static GameObject*  Instantiate();
  static void         Destroy(GameObject* obj);
  static void         DestroyAll();

  GameObject(game_uuid_t id = 0);
  ~GameObject();
  GameObject(GameObject&&);
  GameObject& operator=(GameObject&&);

  // Get a component from this game object. Components are usually retrieved via a 
  // hashed value, which matches their uuid. If a component does not exist in this
  // game object, it will return a nullptr.
  template<typename Obj>
  Obj*                                GetComponent() {
    static_assert(std::is_base_of<Component, Obj>::value, "Type does not inherit Component.");
    component_t uuid = Obj::UUID();
    auto it = m_Components.find(uuid);
    if (it != m_Components.end()) {
      return static_cast<Obj*>(m_Components[uuid].Ptr());
    }
    
    return nullptr; 
  }

  // Add a component to this game object. Request is ignored if a component already
  // exists in this game object, with the same type.
  template<class T>
  void                                AddComponent() {
    static_assert(std::is_base_of<Component, T>::value, "Type does not inherit Component.");
    component_t uuid = T::UUID();
    auto it = m_Components.find(uuid);
    if (it != m_Components.end()) {
      Log(rNotify) << T::GetName() << " already exists in game object. Skipping...\n";
      return; 
    }

    m_Components[uuid] = Component::Create<T>();
    m_Components[uuid]->Initialize(this);
  }

  // Destroys a component from this game object.
  template<typename T>
  void                                DestroyComponent() {
    static_assert(std::is_base_of<Component, T>::value, "Type does not inherit Component.");
    component_t uuid = T::UUID();
    auto it = m_Components.find(uuid);
    if (it != m_Components.end()) {
      m_Components[uuid]->CleanUp();
      m_Components.erase(uuid);
    }
  }

  void                                Serialize(IArchive& archive) override;
  void                                Deserialize(IArchive& archive) override;
  void                                SetParent(GameObject* parent) { m_pParent = parent; }
  void                                SetName(std::string name) { m_Name = name; }

  GameObject*                         GetParent() { return m_pParent; }
  GameObject*                         GetChild(std::string id);
  GameObject*                         GetChild(size_t idx);

  Transform*                          GetTransform() { return GetComponent<Transform>(); }
  std::string                         GetName() const { return m_Name; }
  game_uuid_t                         GetId() const { return m_Id; }

private:
  std::string                         m_Name;

  // The components associated with this game object.
  std::unordered_map<component_t, 
    APtr<Component> >                 m_Components;

  // List of associated children.
  std::vector<GameObject*>            m_Children;

  // Possible parent.
  GameObject*                         m_pParent;
  game_uuid_t                         m_Id;

  friend class GameObjectManager;
  friend class Component;
  friend class Scene;
};
} // Recluse