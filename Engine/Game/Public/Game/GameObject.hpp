// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"

#include "Component.hpp"

#include <unordered_map>

namespace Recluse {

class CBehavior;

// Game Object, used for the game itself. These objects are the fundamental data type
// in our game, which hold important info regarding various data about transformation,
// physics, audio, animation, etc.
class GameObject : public ISerializable {
public:
  GameObject();
  ~GameObject();
  GameObject(const GameObject&);
  GameObject(GameObject&&);

  GameObject& operator=(const GameObject&);
  GameObject& operator=(GameObject&&);

  // Get a component from this game object. Components are usually retrieved via a 
  // hashed value, which matches their uid.
  template<typename Obj>
  Obj                      GetComponent() {
    static_assert(std::is_pointer<Obj>::value, "Must be a pointer type!"); 
    return nullptr; 
  }

  // Add a component to this game object.
  template<typename Obj>
  void                      AddComponent() {
  }

  void                      Serialize(IArchive& archive) override;

  void                      Deserialize(IArchive& archive) override;

  GameObject*               GetParent() { return mParent; }

  GameObject*               GetChild(std::string id);
  GameObject*               GetChild(size_t idx);

  Transform*                GetTransform() { return &mTransform; }

private:
  // Mandatory to have a transform for all game objects.
  Transform                 mTransform;

  // The components associated with this game object.
  std::unordered_map<u64, Component>    mComponents;

  // List of associated children.
  std::vector<GameObject*>  mChildren;

  // Possible parent.
  GameObject*               mParent;
};
} // Recluse