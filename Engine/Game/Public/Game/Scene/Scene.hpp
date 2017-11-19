// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Utility/Vector.hpp"


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
  void                      AddGameObject(GameObject* obj);

  // Remove a game object at a specified index. Returns
  // null if no object was found there.
  GameObject*               RemoveGameObject(u32 idx);

  GameObject*               Get(size_t idx);

  GameObject*               operator[](size_t idx) {
    return Get(idx);
  }

  // Set the name of this scene.
  void                      SetName(std::string name) { m_SceneName = name; }

  // Get the name of this scene!
  std::string               Name() const { return m_SceneName; }

  void                      Serialize(IArchive& archive) override;
  void                      Deserialize(IArchive& archive) override;

private:
  // Metadata.
  // Game objects that are in this scene.
  std::vector<GameObject*>  m_GameObjects;
  std::string               m_SceneName;
};
} // Recluse 