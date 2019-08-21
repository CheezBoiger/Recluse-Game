// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Geometry/Cube.hpp"
#include "Core/Exception.hpp"

#include "Core/Math/Vector4.hpp"

#include <array>


namespace Recluse {


const Vector3 Cube::minimum = Vector3(-1.0f, -1.0f, -1.0f);
const Vector3 Cube::maximum = Vector3( 1.0f,  1.0f,  1.0f);

std::array<Vector4, 36> positions = {
  // front
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
  // up
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
  // right
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
  // front 
  Vector4(0.0f, 0.0f, 1.0f, 0.0f),
  Vector4(0.0f, 0.0f, 1.0f, 0.0f),
  Vector4(0.0f, 0.0f, 1.0f, 0.0f),
  Vector4(0.0f, 0.0f, 1.0f, 0.0f),
  Vector4(0.0f, 0.0f, 1.0f, 0.0f),
  Vector4(0.0f, 0.0f, 1.0f, 0.0f),
  // Back
  Vector4(0.0f, 0.0f, -1.0f, 0.0f),
  Vector4(0.0f, 0.0f, -1.0f, 0.0f),
  Vector4(0.0f, 0.0f, -1.0f, 0.0f),
  Vector4(0.0f, 0.0f, -1.0f, 0.0f),
  Vector4(0.0f, 0.0f, -1.0f, 0.0f),
  Vector4(0.0f, 0.0f, -1.0f, 0.0f),
  // up
  Vector4(0.0f, 1.0f, 0.0f, 0.0f),
  Vector4(0.0f, 1.0f, 0.0f, 0.0f),
  Vector4(0.0f, 1.0f, 0.0f, 0.0f),
  Vector4(0.0f, 1.0f, 0.0f, 0.0f),
  Vector4(0.0f, 1.0f, 0.0f, 0.0f),
  Vector4(0.0f, 1.0f, 0.0f, 0.0f),
  // Down
  Vector4(0.0f, -1.0f, 0.0f, 0.0f),
  Vector4(0.0f, -1.0f, 0.0f, 0.0f),
  Vector4(0.0f, -1.0f, 0.0f, 0.0f),
  Vector4(0.0f, -1.0f, 0.0f, 0.0f),
  Vector4(0.0f, -1.0f, 0.0f, 0.0f),
  Vector4(0.0f, -1.0f, 0.0f, 0.0f),
  // right
  Vector4(1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(1.0f, 0.0f, 0.0f, 0.0f),
  // Left
  Vector4(-1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 0.0f),
  Vector4(-1.0f, 0.0f, 0.0f, 0.0f)
};


std::array<Vector4, 36> texcoords = {
  // front
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

  // right
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


std::array<U32, 36> indices = {
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


std::vector<StaticVertex> Cube::meshInstance(R32 scale)
{
  std::vector<StaticVertex> cube(36);
  for (size_t i = 0; i < cube.size(); ++i) {
    cube[i].position = positions[i] * scale;
    cube[i].position.w = 1.0f;
    cube[i].normal = normals[i];
    cube[i].texcoord0 = Vector2(texcoords[i].x, texcoords[i].y);
    //null_bones(cube[i]);

    cube[i].position.y *= -1.0f;
    cube[i].normal.y *= -1.0f;
  }
  return cube;
}


std::vector<U32> Cube::indicesInstance()
{
  std::vector<U32> cubeIs(36);
  for (size_t i = 0; i < indices.size(); ++i) {
    cubeIs[i] = indices[i];
  }

  return cubeIs;
}
} // Recluse