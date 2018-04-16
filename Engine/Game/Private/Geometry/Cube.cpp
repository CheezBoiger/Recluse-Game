// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Geometry/Cube.hpp"
#include "Core/Exception.hpp"

#include "Core/Math/Vector4.hpp"

#include <array>

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

std::array<Vector4, 36> positions = {
  // Front
  Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
  Vector4( 1.0f, -1.0f, 1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, 1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, 1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, 1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, 1.0f, 1.0f),
  // Back
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  // Up
  Vector4( 1.0f,  1.0f,  1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f,  1.0f, 1.0f),
  Vector4( 1.0f,  1.0f,  1.0f, 1.0f),
  // Down
  Vector4( 1.0f, -1.0f,  1.0f, 1.0f),
  Vector4(-1.0f, -1.0f,  1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4( 1.0f, -1.0f, -1.0f, 1.0f),
  Vector4( 1.0f, -1.0f,  1.0f, 1.0f),
  // Right
  Vector4( 1.0f, -1.0f,  1.0f, 1.0f),
  Vector4( 1.0f, -1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f, -1.0f, 1.0f),
  Vector4( 1.0f,  1.0f,  1.0f, 1.0f),
  Vector4( 1.0f, -1.0f,  1.0f, 1.0f),
  // Left
  Vector4(-1.0f, -1.0f,  1.0f, 1.0f),
  Vector4(-1.0f,  1.0f,  1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f,  1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f, -1.0f, 1.0f),
  Vector4(-1.0f, -1.0f,  1.0f, 1.0f),
};


std::array<Vector4, 36> normals = {
  // Front 
  Vector4(0.0f, 0.0f, 1.0f, 1.0f),
  Vector4(0.0f, 0.0f, 1.0f, 1.0f),
  Vector4(0.0f, 0.0f, 1.0f, 1.0f),
  Vector4(0.0f, 0.0f, 1.0f, 1.0f),
  Vector4(0.0f, 0.0f, 1.0f, 1.0f),
  Vector4(0.0f, 0.0f, 1.0f, 1.0f),
  // Back
  Vector4(0.0f, 0.0f, -1.0f, 1.0f),
  Vector4(0.0f, 0.0f, -1.0f, 1.0f),
  Vector4(0.0f, 0.0f, -1.0f, 1.0f),
  Vector4(0.0f, 0.0f, -1.0f, 1.0f),
  Vector4(0.0f, 0.0f, -1.0f, 1.0f),
  Vector4(0.0f, 0.0f, -1.0f, 1.0f),
  // Up
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  // Down
  Vector4(0.0f, -1.0f, 0.0f, 1.0f),
  Vector4(0.0f, -1.0f, 0.0f, 1.0f),
  Vector4(0.0f, -1.0f, 0.0f, 1.0f),
  Vector4(0.0f, -1.0f, 0.0f, 1.0f),
  Vector4(0.0f, -1.0f, 0.0f, 1.0f),
  Vector4(0.0f, -1.0f, 0.0f, 1.0f),
  // Right
  Vector4(1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(1.0f, 0.0f, 0.0f, 1.0f),
  // Left
  Vector4(-1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 1.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 1.0f)
};


std::array<Vector4, 36> texcoords = {
  // Front
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 1.0f, 0.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),

  Vector4(0.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 1.0f, 0.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),

  Vector4(0.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 1.0f, 0.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),

  Vector4(0.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 1.0f, 0.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),

  // Right
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 1.0f, 0.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),

  // Left
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 1.0f, 0.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(0.0f, 1.0f, 0.0f, 1.0f),
  Vector4(0.0f, 0.0f, 0.0f, 0.0f),
};


std::array<Vector4, 36> colors = {
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),

  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),

  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),

  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),

  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),

  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
  Vector4(1.0f, 1.0f, 1.0f, 1.0f),
};


std::array<u32, 36> indices = {
  0, 1, 2,
  3, 4, 5,
  6, 7, 8,
  9, 10, 11,
  12, 13, 14,
  15, 16, 17,
  18, 19, 20,
  21, 22, 23,
  24, 25, 26,
  27, 28, 29,
  30, 31, 32,
  33, 34, 35
};


std::vector<StaticVertex> Cube::MeshInstance(r32 scale)
{
  std::vector<StaticVertex> cube(36);
  for (size_t i = 0; i < cube.size(); ++i) {
    cube[i].position = positions[i] * scale;
    cube[i].position.w = 1.0f;
    cube[i].position.y *= -1.0f;
    cube[i].normal = normals[i];
    cube[i].normal.y *= -1.0f;
    cube[i].texcoord0 = Vector2(texcoords[i].x, texcoords[i].y);
    //null_bones(cube[i]);
  }
  return cube;
}


std::vector<u32> Cube::IndicesInstance()
{
  std::vector<u32> cubeIs(36);
  for (size_t i = 0; i < indices.size(); ++i) {
    cubeIs[i] = indices[i];
  }

  return cubeIs;
}
} // Recluse