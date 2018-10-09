// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Vector3.hpp"

namespace Recluse {


class TextureCubeArray;
class Texture2DArray;
class DescriptorSet;
class DescriptorSetLayout;
class TextureCube;
class Texture;
class VulkanRHI;
class Renderer;

struct Image;

// Diffuse light probe, used for global illumination lighting in the scene.
struct LightProbe {
  Vector3 _shcoeff[9];   // Spherical harmonic coefficients.
  Vector3 _position;  // Position of where the this probe is, in 3D space.
  r32     _r;         // radius of effect, that this light probe will influence.

  // Generate SH coefficients from texture enviroment cube data.
  void    GenerateSHCoefficients(VulkanRHI* rhi, TextureCube* envMap);

  // Generate coefficients through image data that is not stored in gpu memory.
  void    GenerateSHCoefficients(const Image* img);

  // Save SH data to a file.
  b32     SaveToFile(const std::string& filename);

  // Load up spherical harmonic data from a file.
  b32     LoadFromFile(const std::string& filename);
};


class LightProbeManager {
public:
  static const u32 kMaxAllowedProbes = 128;

  std::vector<LightProbe> GenerateProbes(u32 count = kMaxAllowedProbes);
};


class GlobalIllumination {
public:
  GlobalIllumination();
  ~GlobalIllumination();

  void              Initialize(VulkanRHI* pRhi, b32 enableLocalReflections);

  void              Update(Renderer* pRenderer);

  void              CleanUp(VulkanRHI* pRhi);

  void              SetGlobalEnvMap(Texture* pCube) { m_pGlobalEnvMap = pCube; }
  
  void              SetGlobalBRDFLUT(Texture* pTex) { m_pGlobalBRDFLUT = pTex; }
  // Set up irradiance maps for this renderer to use for look up.
  void              SetIrraMap(TextureCubeArray* maps) { m_pIrrMaps = maps; }

  // Set up enviroment maps for this renderer to use for look up.
  void              SetEnvMaps(TextureCubeArray* maps) { m_pEnvMaps = maps; }

  DescriptorSet*   GetDescriptorSet() { return m_pGlobalIllumination; }

private:
  Texture*              m_pGlobalEnvMap;
  Texture*              m_pGlobalBRDFLUT;
  TextureCubeArray*     m_pEnvMaps;
  TextureCubeArray*     m_pIrrMaps;
  Texture2DArray*       m_pBrdfLUTs;
  DescriptorSet*        m_pGlobalIllumination;
  b32                   m_localReflectionsEnabled;
};
} // Recluse