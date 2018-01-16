// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Utility/Vector.hpp"

#include <set>

namespace Recluse {


class GameObject;


// Scene graph, used for storing, keeping track off, and 
// maintaining, the current state of the game world.
class Scene : public ISerializable {
  static std::string        default_name;
public:
  Scene(std::string name = default_name);
  ~Scene();

  // Add game object into the scene.
  b8                      AddGameObject(GameObject* obj) {
    if (!obj) return false;
    auto it = m_UniqueGameObjects.find(obj->GetId());
    if (it != m_UniqueGameObjects.end()) return false;
    m_UniqueGameObjects.insert(obj->GetId());
    m_GameObjects.push_back(obj);
    return true;
  }

  // Remove a game object at a specified index. Returns
  // null if no object was found there.
  GameObject*               RemoveGameObject(u32 idx) {
    
    return nullptr;
  }

  GameObject*               Get(size_t idx) { return m_GameObjects[idx]; }

  GameObject*               operator[](size_t idx) {
    return Get(idx);
  }

  size_t                    GameObjectCount() const { return m_GameObjects.size(); }

  // Set the name of this scene.
  void                      SetName(std::string name) { m_SceneName = name; }

  // Get the name of this scene!
  std::string               Name() const { return m_SceneName; }

  void                      Serialize(IArchive& archive) override;
  void                      Deserialize(IArchive& archive) override;

private:
  // Metadata.
  // Game objects that are in this scene.
  std::set<game_uuid_t>     m_UniqueGameObjects;
  std::vector<GameObject*>  m_GameObjects;
  std::string               m_SceneName;
  u32                       m_GameObjNum;
};
} // Recluse 