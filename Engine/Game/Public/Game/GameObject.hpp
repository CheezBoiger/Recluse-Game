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
class GameObject : public ISerializable, public IBehavior {

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
    auto it = mComponents.find(uuid);
    if (it != mComponents.end()) {
      return static_cast<Obj*>(mComponents[uuid].Ptr());
    }
    
    return nullptr; 
  }

  // Add a component to this game object.
  template<class T = Component>
  void                                AddComponent() {
    static_assert(std::is_base_of<Component, T>::value, "Type does not inherit Component.");
    component_t uuid = T::UUID();
    if (uuid == Transform::UUID()) {
      Log(rNotify) << Transform::GetName() << " already exists in game object. Skipping...\n";
      return;
    }

    auto it = mComponents.find(uuid);
    if (it != mComponents.end()) {
      Log(rNotify) << T::GetName() << " already exists in game object. Skipping...\n";
      return; 
    }

    mComponents[uuid] = std::move(T());
    mComponents[uuid]->SetOwner(this);
  }

  void                                Serialize(IArchive& archive) override;
  void                                Deserialize(IArchive& archive) override;
  void                                SetParent(GameObject* parent) { mParent = parent; }
  void                                SetName(std::string name) { mName = name; }

  GameObject*                         GetParent() { return mParent; }
  GameObject*                         GetChild(std::string id);
  GameObject*                         GetChild(size_t idx);

  Transform*                          GetTransform() { return &mTransform; }
  std::string                         GetName() const { return mName; }
  game_uuid_t                         GetId() const { return mId; }

private:
  std::string                         mName;
  // Mandatory to have a transform for all game objects.
  Transform                           mTransform;

  // The components associated with this game object.
  std::unordered_map<component_t, 
    APtr<Component> >                 mComponents;

  // List of associated children.
  std::vector<GameObject*>            mChildren;

  // Possible parent.
  GameObject*                         mParent;
  game_uuid_t                         mId;

  friend class GameObjectManager;
};
} // Recluse