// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Quaternion.hpp"

namespace Recluse {


class Mesh;


struct RenderCmd {
  enum RenderCmdType {
    RENDER_CMD_NONE,
    RENDER_CMD_TRANSFORM,
    RENDER_CMD_MESH,
    RENDER_CMD_DIRECTIONLIGHT,
    RENDER_CMD_POINTLIGHT,
    RENDER_CMD_SPOTLIGHT
  };

  RenderCmd(RenderCmdType type = RENDER_CMD_NONE)
    : type(type) { }

  RenderCmdType     Type() const { return type; }

private:
  RenderCmdType     type;
};


struct TransformCmd : public RenderCmd {
  TransformCmd()
    : RenderCmd(RENDER_CMD_TRANSFORM) { }
   
  Quaternion  rotation;
  Vector3     position;
  Vector3     scale;
};


struct DirectionlightCmd : public RenderCmd {
  DirectionlightCmd()
    : RenderCmd(RENDER_CMD_DIRECTIONLIGHT) { }

  Vector4 color;
  Vector3 direction;
  r32     intensity;
};


struct PointLightCmd : public RenderCmd {
  PointLightCmd()
    : RenderCmd(RENDER_CMD_POINTLIGHT) { }

  Vector4 color;
  Vector3 position;
  r32     radius;
  r32     intensity;  
};


struct SpotLightCmd : public RenderCmd {
  SpotLightCmd()
    : RenderCmd(RENDER_CMD_SPOTLIGHT) { }
  
  Vector4 color;
  Vector3 position;
};


struct MeshCmd : public RenderCmd {
  MeshCmd()
    : RenderCmd(RENDER_CMD_MESH) { }
  
  Mesh*   meshId;
};
} // Recluse