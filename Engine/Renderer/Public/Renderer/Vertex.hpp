// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Vector2.hpp"
#include "Core/Math/Quaternion.hpp"


#define load_position_v4(m, p) { \
  m.position[0] = p.x; m.position[1] = p.y; m.position[2] = p.z; m.position[3] = p.w; \
} 

#define load_normal_v4(m, n) { \
  m.normal[0] = n.x; m.normal[1] = n.y; m.normal[2] = n.z; m.normal[3] = n.w; \
}


#define load_texcoords(m, t) { \
  m.texcoord0[0] = t.x; m.texcoord0[1] = t.y; \
  m.texcoord1[0] = t.z; m.texcoord1[1] = t.z; \
}


#define load_color(m, c) { \
  m.color[0] = c.x; m.color[1] = c.y; m.color[2] = c.z; m.color[3] = c.w; \
}

#define null_bones(m) { \
  m.boneWeights.x = 0.0f; m.boneWeights.y = 0.0f; m.boneWeights.z = 0.0f; m.boneWeights.w = 0.0f; \
  m.boneIds[0] = 0; m.boneIds[1] = 0; m.boneIds[2] = 0; m.boneIds[3] = 0; \
}


namespace Recluse {


// NOTE(): These vertex classes need to be known to the renderer!
// They are defined in VertexDescription.hpp, in private!


struct StaticVertex {
  Vector4 position;
  Vector4 normal;
  Vector2 texcoord0;
  Vector2 texcoord1;

  bool operator==(const StaticVertex& other) const {
    return position == other.position && normal == other.normal 
        && texcoord0 == other.texcoord0 && texcoord1 == other.texcoord1;
  }
};


struct QuadVertex {
  Vector2 position;  
  Vector2 texcoord;
};


struct UIVertex {
  Vector2 position;
  Vector2 texcoord;
  Vector4 color;
};


struct SkinnedVertex {
  Vector4 position;
  Vector4 normal;
  Vector2 texcoord0;
  Vector2 texcoord1;
  Vector4 boneWeights;
  I32     boneIds       [4];
};


// Morph vertices are the same as static vertices.
using MorphVertex = StaticVertex;


// For Meshes with no normal data needed.
struct VertexNoNormal {
  Vector4 position;
  Vector2 uv0;
  Vector2 uv1;
};


// Per instance information.
struct DecalPerInstanceInfo {
  Vector4       position;
  Quaternion    rotation;
  Vector4       scale;
  Vector4       textureInfo;  // x -> lodBias, y -> textureIndex within a Texture array.
};
} // Recluse