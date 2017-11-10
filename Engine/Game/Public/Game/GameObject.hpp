// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


class Component;

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

  GameObject*               Parent() { return mParent; }

  GameObject*               Child(std::string id);
  GameObject*               Child(size_t idx);

private:
  // The components associated with this game object.
  std::vector<Component*>   mComponents;

  // List of associated children.
  std::vector<GameObject*>  mChildren;

  // Possible parent.
  GameObject*               mParent;
};
} // Recluse