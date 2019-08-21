// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Math/Common.hpp"
#include "Geometry/UVSphere.hpp"

#define null_bones(m) { \
  m.boneWeights.x = 0.0f; m.boneWeights.y = 0.0f; m.boneWeights.z = 0.0f; m.boneWeights.w = 0.0f; \
  m.boneIds[0] = 0; m.boneIds[1] = 0; m.boneIds[2] = 0; m.boneIds[3] = 0; \
}

namespace Recluse {

std::vector<StaticVertex> UVSphere::meshInstance(R32 radius, U32 sliceCount, U32 stackCount)
{
  //subdivisions = (std::min)(subdivisions, 5u);
  std::vector<StaticVertex> vertices;

  //
  // Compute the vertices stating at the top pole and moving down the stacks.
  //

  // Poles: note that there will be texture coordinate distortion as there is
  // not a unique point on the texture map to assign to the pole when mapping
  // a rectangular texture onto a sphere.
  StaticVertex topVertex;
  Vector4 color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

  topVertex.position = Vector4(0.0f, radius, 0.0f, 1.0f);
  topVertex.normal = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
  topVertex.texcoord0 = Vector2(0.0f, 0.0f);
  topVertex.texcoord1 = Vector2(0.0f, 0.0f);
  //null_bones(topVertex);

  StaticVertex bottomVertex;
  bottomVertex.position = Vector4(0.0f, -radius, 0.0f, 1.0f);
  bottomVertex.normal = Vector4(0.0f, -1.0f, 0.0f, 1.0f);
  bottomVertex.texcoord0 = Vector2(0.0f, 1.0f);
  bottomVertex.texcoord1 = Vector2(0.0f, 1.0f);
  //null_bones(bottomVertex);

  vertices.push_back(bottomVertex);

  float phiStep = R32(CONST_PI) / stackCount;
  float thetaStep = 2.0f * R32(CONST_PI) / sliceCount;

  // Compute vertices for each stack ring (do not count the poles as rings).
  for (U32 i = 1; i <= stackCount - 1; ++i)
  {
    R32 phi = i*phiStep;

    // Vertices of ring.
    for (U32 j = 0; j <= sliceCount; ++j)
    {
      R32 theta = j*thetaStep;

      StaticVertex v;
      //null_bones(v);

      // spherical to cartesian
      v.position.x = radius*sinf(phi)*cosf(theta);
      v.position.y = radius*cosf(phi);
      v.position.z = radius*sinf(phi)*sinf(theta);
      v.position.w = 1.0f;

      Vector3 p = Vector3(v.position.x, v.position.y, v.position.z);
      v.normal = Vector4(p.normalize(), 1.0f);
  
      v.texcoord0.x = theta / R32(CONST_2_PI);
      v.texcoord0.y = phi / R32(CONST_PI);
      v.texcoord1 = v.texcoord0;

      v.normal.y *= -1.0f;
      v.position.y *= -1.0f;
      vertices.push_back(v);
    }
  }

  vertices.push_back(topVertex);

  return vertices;
}


std::vector<U32> UVSphere::indicesInstance(U32 verticesCnt, U32 sliceCount, U32 stackCount)
{
  std::vector<U32> indices;
  //
  // Compute indices for top stack.  The top stack was written first to the vertex buffer
  // and connects the top pole to the first ring.
  //

  for (uint32_t i = 1; i <= sliceCount; ++i)
  {
    indices.push_back(0);
    indices.push_back(i + 1);
    indices.push_back(i);
  }

  //
  // Compute indices for inner stacks (not connected to poles).
  //

  // Offset the indices to the index of the first vertex in the first ring.
  // This is just skipping the top pole vertex.
  U32 baseIndex = 1;
  U32 ringVertexCount = sliceCount + 1;
  for (U32 i = 0; i < stackCount - 2; ++i)
  {
    for (U32 j = 0; j < sliceCount; ++j)
    {
      indices.push_back(baseIndex + i*ringVertexCount + j);
      indices.push_back(baseIndex + i*ringVertexCount + j + 1);
      indices.push_back(baseIndex + (i + 1)*ringVertexCount + j);

      indices.push_back(baseIndex + (i + 1)*ringVertexCount + j);
      indices.push_back(baseIndex + i*ringVertexCount + j + 1);
      indices.push_back(baseIndex + (i + 1)*ringVertexCount + j + 1);
    }
  }

  //
  // Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
  // and connects the bottom pole to the bottom ring.
  //

  // South pole vertex was added last.
  U32 southPoleIndex = (U32)verticesCnt - 1;

  // Offset the indices to the index of the first vertex in the last ring.
  baseIndex = southPoleIndex - ringVertexCount;

  for (U32 i = 0; i < sliceCount; ++i)
  {
    indices.push_back(southPoleIndex);
    indices.push_back(baseIndex + i);
    indices.push_back(baseIndex + i + 1);
  }

  return indices;
}
} // Recluse