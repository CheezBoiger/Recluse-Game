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
    : target(nullptr)
    , debug(false) { }


  RenderObject*     target;
  b8                debug;
};
} // Recluse