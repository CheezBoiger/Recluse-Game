// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


class GameObject;


struct SceneNode {
  std::vector<SceneNode*> m_Children;
  SceneNode*              m_Parent;
  GameObject*             m_GameObject;
};


// Scene graph, used for storing, keeping track off, and 
// maintaining, the current state of the game world.
class Scene {
public:

};
} // Recluse 