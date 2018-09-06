// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Vector3.hpp"

namespace Recluse {


class TextureCube;
class VulkanRHI;

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
} // Recluse