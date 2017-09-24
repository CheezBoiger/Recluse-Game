// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"

namespace Recluse {


struct RenderCmd {
  enum RenderCmdType {
    RENDER_CMD_NONE,
    RENDER_CMD_TRANSFORM,
    RENDER_CMD_MESH,
    RENDER_CMD_DIRECTIONLIGHT,
    RENDER_CMD_POINTLIGHT,
    RENDER_CMD_SPOTLIGHT
  };

  RenderCmd(RenderCmdType type)
    : Type(type) { }

  RenderCmdType     Type;
};



struct MeshCmd : public RenderCmd {
  MeshCmd()
    : RenderCmd(RENDER_CMD_MESH) { }

};


struct TransformCmd : public RenderCmd {
  TransformCmd()
    : RenderCmd(RENDER_CMD_TRANSFORM) { }
   
  Quaternion  Rotation;
  Vector3     Position;
  Vector3     Scale;
};


struct DirectionlightCmd : public RenderCmd {
  DirectionlightCmd()
    : RenderCmd(RENDER_CMD_DIRECTIONLIGHT) { }

  Vector3 Direction;
  Vector4 Color;
};
} // Recluse