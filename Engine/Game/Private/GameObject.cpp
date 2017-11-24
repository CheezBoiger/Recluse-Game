// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "GameObject.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


GameObject::GameObject()
  : mParent(nullptr)
{
}


GameObject::~GameObject()
{
}


GameObject::GameObject(const GameObject& obj)
{
}


GameObject::GameObject(GameObject&& obj)
{
}


GameObject& GameObject::operator=(const GameObject& obj)
{
  return (*this);
}


GameObject& GameObject::operator=(GameObject&& obj)
{
  return (*this);
}


void GameObject::Serialize(IArchive& archive)
{
}


void GameObject::Deserialize(IArchive& archive)
{

}
} // Recluse