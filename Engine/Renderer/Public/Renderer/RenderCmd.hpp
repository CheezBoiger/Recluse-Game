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

struct Primitive;


enum CmdConfig {
  CMD_RENDERABLE_BIT  = (1 << 0),
  CMD_DEBUG_BIT       = (1 << 2),
  CMD_SHADOWS_BIT     = (1 << 3),
  CMD_INSTANCING_BIT  = (1 << 4),
  CMD_TRANSLUCENT_BIT = (1 << 5),
  CMD_TRANSPARENT_BIT = (1 << 6),
  CMD_FORWARD_BIT     = (1 << 7),
  CMD_ALLOW_CULL_BIT  = (1 << 8)
};

  
typedef u32 CmdConfigBits;


struct MeshRenderCmd {
  MeshRenderCmd()
    : _config(false)
    , _pMeshDesc(nullptr)
    , _pPrimitives(nullptr)
    , _pMeshData(nullptr)
    , _instances(1) { }


  MeshData*               _pMeshData;
  MeshDescriptor*         _pMeshDesc;
  Primitive*              _pPrimitives;
  u32                     _primitiveCount;
  u32                     _instances;
  CmdConfigBits           _config;
};


struct UiRenderCmd {
  UiRenderCmd()
    : _config(false) { }

  CmdConfigBits            _config;
};
} // Recluse