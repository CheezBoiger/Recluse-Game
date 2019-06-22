// Copyright (c) 2019 Recluse Project. All rights reserved.
#ifndef CLUSTERING_GLSL
#define CLUSTERING_GLSL

#include "Globals.glsl"
#include "LightingPBR.glsl"

#define NAN 0./0.


struct LightBVH {
  uint  numLevels;
  uint  numNodes;
  uint  numMaxNodes;
  uint  numMaxLights;
  vec4  zGridLocFar;
  vec4  nodesMin[128];
  vec4  nodesMax[128];
  uvec4 nodeIndices[128];
};


struct GridData {
  // Offset points to the first light source in the tile, followed by other counts for decals and probes.
  //
  //               |-----32------|--------------------------32--------------------------|    
  //               |             |                                                      |    
  //               |             |                                                      |    
  //               |             |                                                      |
  ivec2  param; // [ offset (32) | light count (16) | decal count (8) | probe count (8) ]
};


///////////////////////////////////////////////////////////////////////////
// Light BVH functions.
///////////////////////////////////////////////////////////////////////////

uint GetLevelStart(uint level)
{
  switch (level) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 33;
    case 3: return 1057;
    case 4: return 33825;
    case 5: return 1082401;
  }
  return 0;
}


void SetNodeLightBVH(inout LightBVH lightBVH, uint level, uint nodeId, vec3 nodeMin, vec3 nodeMax)
{
  uint levelStart = GetLevelStart(level);
  uint nodeIndex = levelStart + nodeId;
  if ( gl_GlobalInvocationID.x == 0 ) {
    lightBVH.nodesMin[nodeIndex] = vec4(nodeMin, 0.0);
    lightBVH.nodesMax[nodeIndex] = vec4(nodeMax, 0.0);
  }
}


void SetLeafLightBVH(inout LightBVH lightBVH, uint leafId, uint lightId) 
{
  if ( leafId < lightBVH.numNodes ) {
    lightBVH.nodeIndices[leafId].x = lightId;
  }
}


vec3 GetNodeMin(inout LightBVH lightBVH, uint idx)
{
  return lightBVH.nodesMin[idx].xyz;
}


vec3 GetNodeMax(inout LightBVH lightBVH, uint idx)
{
  return lightBVH.nodesMax[idx].xyz;
}


void SetRoot(inout LightBVH lightBVH, vec4 rootMin, vec4 rootMax)
{
  lightBVH.nodesMin[0] = rootMin;
  lightBVH.nodesMax[0] = rootMax;
}


///////////////////////////////////////////////////////////////////////////
// Cluster building.
///////////////////////////////////////////////////////////////////////////


void buildClusters(in GlobalBuffer global, int numLights)
{
  uint gridDimY = 0;
  uint grid2dDimY = (global.screenSize.y + gridDimY - 1) / gridDimY; 
}

///////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////

vec4 SphereAABBMinOP(vec4 aa, vec4 ab)
{
  return vec4(min(aa.x, ab.x),
              min(aa.y, ab.y),
              min(aa.z, ab.z),
              0.0);
}


vec4 SphereAABBMaxOP(vec4 aa, vec4 ab)
{
  return vec4(max(aa.x, ab.x),
              max(aa.y, ab.y),
              max(aa.z, ab.z),
              0.0);
}


bool Overlaps(const vec4 aaMin, const vec4 aaMax, const vec4 abMin, const vec4 abMax)
{
  return aaMax.x > abMin.x && aaMin.x < abMax.x
      && aaMax.y > abMin.y && aaMin.y < abMax.y
      && aaMax.z > abMin.z && aaMin.z < abMax.z;
}


PointLight GetLightRange(in LightBuffer lights, uint idx)
{
  if (idx >= MAX_POINT_LIGHTS) {
    PointLight l;
    l.position = vec4(NAN, NAN, NAN, NAN);
    return l;
  }
  return lights.pointLights[idx];
}


void lightHierarchyMakeKeys(in LightBuffer lights)
{
  uint index = gl_GlobalInvocationID.x;
  if (index >= MAX_POINT_LIGHTS) return;
  const uint kCoordBits = 8;
  const uint kCoordScale = (1u<<kCoordBits) - 1u; 
  
  PointLight sphere = GetLightRange(lights, index);
  if (sphere.position.x == NAN) return;
}

#endif // CLUSTERING_GLSL