// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Component.hpp"

#include "Renderer/Mesh.hpp"
#include "Renderer/Material.hpp"


namespace Recluse {


class RenderObject;
class Material;
class Mesh;


class MeshComponent : public Component {
public:
  MeshComponent() { }
  Material      m_Material;
  Mesh          m_Mesh;

private:
  RenderObject* mRenderObj;
};
} // Recluse