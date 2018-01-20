// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GameObject.hpp"
#include "Core/Exception.hpp"

#include "GameObjectManager.hpp"

namespace Recluse {


GameObject* GameObject::Instantiate()
{
  return gGameObjectManager().Allocate();
}


void GameObject::DestroyAll()
{
  gGameObjectManager().Clear();
}


GameObject::GameObject(game_uuid_t id)
  : m_pParent(nullptr)
  , m_Id(id)
  , m_Name("Default Name")
{
}


GameObject::~GameObject()
{
  for (auto& component : m_Components) {
    component.second->CleanUp();
  }
}


GameObject::GameObject(GameObject&& obj)
  : m_Id(obj.m_Id)
  , m_pParent(obj.m_pParent)
  , m_Components(std::move(obj.m_Components))
  , m_Children(std::move(obj.m_Children))
  , m_Name(std::move(obj.m_Name))
{
  obj.m_Id = 0;
  obj.m_pParent = nullptr;
  obj.m_Components.clear();
  obj.m_Children.clear();
  obj.m_Name.clear();
}


GameObject& GameObject::operator=(GameObject&& obj)
{
  m_Id = obj.m_Id;
  m_pParent = obj.m_pParent;
  m_Name = std::move(obj.m_Name);
  m_Components = std::move(obj.m_Components);
  m_Children = std::move(obj.m_Children);

  obj.m_Id = 0;
  obj.m_pParent = nullptr;
  obj.m_Components.clear();
  obj.m_Children.clear();
  return (*this);
}


void GameObject::Update()
{
  for (auto& it : m_Components) {
    it.second->Update();
  }
}


void GameObject::Serialize(IArchive& archive)
{
}


void GameObject::Deserialize(IArchive& archive)
{

}
} // Recluse