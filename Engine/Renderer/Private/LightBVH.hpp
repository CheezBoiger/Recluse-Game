// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "RHI/VulkanRHI.hpp"
#include "Core/Math/AABB.hpp"
#include "Core/Math/Ray.hpp"
#include "Core/Types.hpp"

#include <vector>


#define MAX_LIGHT_GRID_X_TILE 64
#define MAX_LIGHT_GRID_Y_TILE 64
#define MAX_LIGHT_GRID_Z_TILE 256 

namespace Recluse {


class ComputePipeline;
class DescriptorSet;
class DescriptorSetLayout;
class Buffer;
class CommandBuffer;


struct ClusterKey {
  static const U32 g_invalidIdPacked = 0xffffffff;

  static const U32 x_bits = 8;
  static const U32 y_bits = 8;
  static const U32 z_bits = 10;

  static const U32 x_shift = y_bits + z_bits;
  static const U32 y_shift = z_bits;
  static const U32 z_shift = 0;

  static const U32 x_mask = (1ul<<x_bits)-1ul;
  static const U32 y_mask = (1ul<<y_bits)-1ul;
  static const U32 z_mask = (1ul<<z_bits)-1ul;

  U32 _x;
  U32 _y;
  U32 _z;
  U32 _pN; // packed normal.
};


struct IVector4 {
  U32 x,
      y,
      z,
      w;
};


// Cpu bound light bounding volume hierarchy.
struct LightBVH {
  U32       _numLevels;
  U32       _numNodes;
  U32       _numMaxNodes;
  U32       _numMaxLights;
  Vector4   _zGridLocFar;
  Vector4*  _nodesMin;
  Vector4*  _nodesMax;
  IVector4* _lightIndices;
};


class LightHierarchy {
public:
  LightHierarchy();

  void initialize(VulkanRHI* pRhi, U32 width, U32 height);
  void cleanUp(VulkanRHI* pRhi);
  void build(CommandBuffer* pCmdBuffer);
  
private:
  Buffer* m_gpuBvh;
  Buffer* m_gpuClusterGrid;
  Buffer* m_gpuLightIndexList;
  DescriptorSet*  m_descSet;
  DescriptorSetLayout* m_descLayout;
  ComputePipeline* m_pLightAssignment;
  LightBVH m_cpuBvh;
};


struct ProbeBVH {

};
} // Recluse