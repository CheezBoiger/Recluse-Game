// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"

#include <vector>

namespace Recluse {


class MaterialDescriptor;
class MeshDescriptor;
class RenderMaterial;
class MeshData;


enum CmdConfigBits {
  CMD_RENDERABLE_BIT = (1 << 0),
  CMD_DEBUG_BIT = (1 << 2),
  CMD_SHADOWS_BIT = (1 << 3),
  CMD_INSTANCING_BIT = (1 << 4)
};


struct MeshRenderCmd {
  MeshRenderCmd()
    : _config(false)
    , _pMeshDesc(nullptr)
    , _pMatDesc(nullptr)
    , _pMeshData(nullptr)
    , _instances(1) { }


  MeshData*               _pMeshData;
  MeshDescriptor*         _pMeshDesc;
  MaterialDescriptor*     _pMatDesc;
  u32                     _config;
  u32                     _instances;
};


struct UiRenderCmd {
  
};
} // Recluse