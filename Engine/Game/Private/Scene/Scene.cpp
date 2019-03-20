// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "Scene/Scene.hpp"

namespace Recluse {


std::string Scene::default_name = "Default Scene";


Scene::Scene(std::string name)
  : m_SceneName(name) 
{
  m_Root.setSceneOwner(this);
}


void SceneNode::addChild(GameObject* child)
{
  child->setSceneOwner(m_pScene);
  m_GameObjects.push_back(child);
}
} // Recluse