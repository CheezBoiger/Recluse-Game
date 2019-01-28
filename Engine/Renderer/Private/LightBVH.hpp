// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Math/AABB.hpp"
#include "Core/Math/Ray.hpp"
#include "Core/Types.hpp"

#include <vector>


namespace Recluse {


class ComputePipeline;
class DescriptorSet;
class DescriptorSetLayout;
class Buffer;


struct ClusterKey {
  static const u32 g_invalidIdPacked = 0xffffffff;

  static const u32 x_bits = 8;
  static const u32 y_bits = 8;
  static const u32 z_bits = 10;

  static const u32 x_shift = y_bits + z_bits;
  static const u32 y_shift = z_bits;
  static const u32 z_shift = 0;

  static const u32 x_mask = (1ul<<x_bits)-1ul;
  static const u32 y_mask = (1ul<<y_bits)-1ul;
  static const u32 z_mask = (1ul<<z_bits)-1ul;

  u32 _x;
  u32 _y;
  u32 _z;
  u32 _pN; // packed normal.
};


// Cpu bound light bounding volume hierarchy.
struct LightBVH {
  u32       _numLevels;
  u32       _numNodes;
  u32       _numMaxNodes;
  u32       _numMaxLights;
  Vector4*  _nodesMin;
  Vector4*  _nodesMax;
  u32*      _lightIndices;
};


class LightHierarchy {
public:

private:
  Buffer*         m_bvh;
  DescriptorSet*  m_descSet;
  LightBVH        m_cpuBvh;
};


struct ProbeBVH {

};
} // Recluse