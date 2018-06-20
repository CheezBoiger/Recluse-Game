// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "Scene/Scene.hpp"
#include "Game/GameObject.hpp"

namespace Recluse {


std::string Scene::default_name = "Default Scene";


void SceneNode::AddChild(GameObject* child)
{
  m_GameObjects.push_back(child);
  child->SetSceneOwner(m_pScene);
}
} // Recluse