// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Utility/Vector.hpp"

#include "Game/GameObject.hpp"

#include <set>

namespace Recluse {


class GameObject;


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
  GameObject*               GetRoot() { return &m_Root; }

  // Get the name of this scene!
  std::string               Name() const { return m_SceneName; }

  void                      Serialize(IArchive& archive) override { }
  void                      Deserialize(IArchive& archive) override  { }

private:
  std::string               m_SceneName;
  GameObject                m_Root;
};
} // Recluse 