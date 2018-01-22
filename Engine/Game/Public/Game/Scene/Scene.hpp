// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Utility/Vector.hpp"

#include "Game/GameObject.hpp"
#include "Renderer/LightDescriptor.hpp"

#include <set>

namespace Recluse {


class GameObject;


class SceneNode {
public:
  void                      AddChild(GameObject* child) { m_GameObjects.push_back(child); }

  size_t                    GetChildCount() const { return m_GameObjects.size(); }

  GameObject*               GetChild(size_t idx) { return m_GameObjects[idx]; }

private:
  std::vector<GameObject*> m_GameObjects;
};


// Scene graph, used for storing, keeping track off, and 
// maintaining, the current state of the game world.
class Scene : public ISerializable {
  static std::string        default_name;
public:
  Scene(std::string name = default_name)
    : m_SceneName(name) { }
  ~Scene() { }

  // Set the name of this scene.
  void                      SetName(std::string name) { m_SceneName = name; }
  SceneNode*                GetRoot() { return &m_Root; }

  // Get the name of this scene!
  std::string               Name() const { return m_SceneName; }

  void                      Serialize(IArchive& archive) override { }
  void                      Deserialize(IArchive& archive) override  { }

  DirectionalLight*         GetPrimaryLight() { return &m_PrimaryLight; }

  
private:
  DirectionalLight          m_PrimaryLight;
  std::string               m_SceneName;
  SceneNode                 m_Root;

  // Physics based information may go here as well.
};
} // Recluse 