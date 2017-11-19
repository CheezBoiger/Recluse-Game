// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Component.hpp"


namespace Recluse {


class RenderObject;
class Material;
class Mesh;


class MeshComponent : public Component {
public:
  MeshComponent()
    : m_Material(nullptr)
    , m_Mesh(nullptr) { }

  Material*   m_Material;
  Mesh*       m_Mesh;

private:
};
} // Recluse