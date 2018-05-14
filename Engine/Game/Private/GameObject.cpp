// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GameObject.hpp"
#include "Core/Exception.hpp"

#include "GameObjectManager.hpp"

namespace Recluse {


game_uuid_t GameObject::sGameObjectCount = 1;


GameObject::GameObject()
  : m_pParent(nullptr)
  , m_id(std::hash<game_uuid_t>()(sGameObjectCount++))
  , m_name("Default Name")
  , m_bStarted(false)
{
  m_transform.Initialize(this);
}


GameObject::~GameObject()
{
  m_transform.CleanUp();
}


GameObject::GameObject(GameObject&& obj)
  : m_id(obj.m_id)
  , m_pParent(obj.m_pParent)
  , m_children(std::move(obj.m_children))
  , m_name(std::move(obj.m_name))
{
  obj.m_id = 0;
  obj.m_pParent = nullptr;
  obj.m_children.clear();
  obj.m_name.clear();
}


GameObject& GameObject::operator=(GameObject&& obj)
{
  m_id = obj.m_id;
  m_pParent = obj.m_pParent;
  m_name = std::move(obj.m_name);
  m_children = std::move(obj.m_children);

  obj.m_id = 0;
  obj.m_pParent = nullptr;
  obj.m_children.clear();
  return (*this);
}


void GameObject::Serialize(IArchive& archive)
{
}


void GameObject::Deserialize(IArchive& archive)
{

}
} // Recluse