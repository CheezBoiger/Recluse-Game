// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"

namespace Recluse {


class Mesh;
class Material;
class RenderObject;


struct RenderCmd {
  RenderCmd()
    : _pTarget(nullptr)
    , _Debug(false) { }


  RenderObject*     _pTarget;
  b8                _Debug;
};
} // Recluse