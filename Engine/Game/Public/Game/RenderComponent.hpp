// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Component.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/Material.hpp"


namespace Recluse {


class RenderComponent : public Component {
public:
  RenderComponent()
    : m_Material(nullptr)
    , m_Mesh(nullptr) { }


  Material*   m_Material;
  Mesh*       m_Mesh;
};


class SkinnedRenderComponent : public RenderComponent {
public:
  SkinnedRenderComponent()
    : RenderComponent() { }

};
} // Recluse