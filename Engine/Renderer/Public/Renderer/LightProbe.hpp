// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/AABB.hpp"

namespace Recluse {


class TextureCubeArray;
class Texture2DArray;
class DescriptorSet;
class DescriptorSetLayout;
class TextureCube;
class Texture;
class VulkanRHI;
class Renderer;
class Buffer;

struct Image;

// Diffuse light probe, used for global illumination lighting in the scene.
struct LightProbe {
  Vector3 _shcoeff[9];   // Spherical harmonic coefficients.
  Vector3 _position;  // _position of where the this probe is, in 3D space.
  AABB    _aabb;    // AABB bounds.
  r32     _bias;      // bias.

  LightProbe()
    : _bias(1.0f) { }

  // Generate SH coefficients from texture enviroment cube data.
  void    generateSHCoefficients(VulkanRHI* rhi, TextureCube* envMap);

  // Generate coefficients through image data that is not stored in gpu memory.
  void    generateSHCoefficients(const Image* img);

  // Save SH data to a file.
  b32     saveToFile(const std::string& filename);

  // Load up spherical harmonic data from a file.
  b32     loadFromFile(const std::string& filename);
};


class LightProbeManager {
public:
  static const u32 kMaxAllowedProbes = 128;

  std::vector<LightProbe> generateProbes(u32 count = kMaxAllowedProbes);
};


struct DiffuseSH {
  Vector4 _c[9];
  Vector4 _bias;
};


struct LocalInfoGI {
  Vector4 _positions[32];
  Vector4 _minAABB[32];
  Vector4 _maxAABB[32];
  DiffuseSH _diffuse[32];
};


class GlobalIllumination {
public:
  GlobalIllumination();
  ~GlobalIllumination();

  void initialize(VulkanRHI* pRhi, b32 enableLocalReflections);

  void update(Renderer* pRenderer);

  void cleanUp(VulkanRHI* pRhi);

  void setGlobalEnvMap(Texture* pCube) { m_pGlobalEnvMap = pCube; }
  
  void setGlobalBRDFLUT(Texture* pTex) { m_pGlobalBRDFLUT = pTex; }
  void setGlobalProbe(LightProbe* probe) { 
    if (!probe) {
      for (u32 i = 0; i < 9; ++i) {
        m_globalDiffuseSH._c[i] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
      }
      m_globalDiffuseSH._bias = 1.0f;
      return;
    }
    for (u32 i = 0; i < 9; ++i) {
      m_globalDiffuseSH._c[i] = probe->_shcoeff[i];
    }
    m_globalDiffuseSH._bias = probe->_bias;
  }

  // Set up enviroment maps for this renderer to use for look up.
  void setEnvMaps(TextureCubeArray* maps) { m_pEnvMaps = maps; }

  DescriptorSet* getDescriptorSet() { return m_pGlobalIllumination; }

private:
  void updateGlobalGI(VulkanRHI* pRhi);

  Texture*              m_pGlobalEnvMap;
  Texture*              m_pGlobalBRDFLUT;
  Buffer*               m_pGlobalGIBuffer;
  Buffer*               m_pLocalGIBuffer;
  TextureCubeArray*     m_pEnvMaps;
  Texture2DArray*       m_pBrdfLUTs;
  DescriptorSet*        m_pGlobalIllumination;
  DiffuseSH             m_globalDiffuseSH;
  LocalInfoGI           m_localGICpu;
  b32                   m_localReflectionsEnabled;
};
} // Recluse