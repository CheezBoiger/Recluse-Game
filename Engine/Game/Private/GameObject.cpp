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
  : mParent(nullptr)
  , mId(id)
  , mName("Default Name")
{
}


GameObject::~GameObject()
{
}


GameObject::GameObject(GameObject&& obj)
  : mId(obj.mId)
  , mParent(obj.mParent)
  , mComponents(std::move(obj.mComponents))
  , mChildren(std::move(obj.mChildren))
  , mName(std::move(obj.mName))
{
  obj.mId = 0;
  obj.mParent = nullptr;
  obj.mComponents.clear();
  obj.mChildren.clear();
  obj.mName.clear();
}


GameObject& GameObject::operator=(GameObject&& obj)
{
  mId = obj.mId;
  mParent = obj.mParent;
  mComponents = std::move(obj.mComponents);
  mChildren = std::move(obj.mChildren);

  obj.mId = 0;
  obj.mParent = nullptr;
  obj.mComponents.clear();
  obj.mChildren.clear();
  return (*this);
}


void GameObject::Serialize(IArchive& archive)
{
}


void GameObject::Deserialize(IArchive& archive)
{

}
} // Recluse