// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/AABB.hpp"


namespace Recluse {



class DescriptorSet;
class DescriptorSetLayout;
class Buffer;
class VulkanRHI;

// parameters to be pushed to hdr pass. memory size must be kept low!
struct ParamsHDR {
  ParamsHDR() 
    : _bloomStrength(1.0) { }

  R32 _bloomStrength;
};

// Configuration for the downscale blurring pass.
struct BloomConfig
{
  I32 _sz[4];
  Vector4 _invOutputSz;
  Vector4 _threshold;
};

// Realtime configurations of hdr settings.
struct ConfigHDR {
  Vector4 _allowChromaticAberration;
  Vector4 _k;
  Vector4 _kcube;
  Vector4 _bEnable; // x = interleaving, y = fade, z = filmgrain, w = distort.
  Vector4 _interleavedVideoShakeInterval;
  Vector4 _fade;
  Vector4 _filmGrain;
};


class HDR {
public:
  HDR();

  ~HDR();

  void                    initialize(VulkanRHI* pRhi);
  void                    cleanUp(VulkanRHI* pRhi);

  void                    UpdateToGPU(VulkanRHI* pRhi);

  ConfigHDR*              getRealtimeConfiguration() { return &m_config; }
  DescriptorSet*          getSet() const { return m_pSet; };
  DescriptorSetLayout*    getSetLayout() const { return m_pLayout; }

private:

  ConfigHDR               m_config;
  DescriptorSet*          m_pSet;
  DescriptorSetLayout*    m_pLayout;
  Buffer*                 m_pBuffer;
};
} // Recluse