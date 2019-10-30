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
  Vector4 _invOutputSz;
  Vector4 _threshold;
};

struct BrightFilterParameters
{
    // IMPORTANT: Must have this layout, otherwise hdr pass may pass undefined data.
    R32 brightnessThreshold2x;
    R32 brightnessThreshold4x;
    R32 brightnessThreshold8x;
    R32 brightnessThreshold16x;

    // Rest is fine.
    R32 bloomStrength16x;
    R32 bloomScale16x;
    R32 bloomStrength8x;
    R32 bloomScale8x;
    R32 bloomStrength4x;
    R32 bloomScale4x;
    R32 bloomStrength2x;
    R32 bloomScale2x;
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
  BrightFilterParameters*        getBrightFilterParams() { return &m_brightFilterParams; }
  void setBrightFilterParams(const BrightFilterParameters& params) { m_brightFilterParams = params; }

private:
  BrightFilterParameters  m_brightFilterParams;
  ConfigHDR               m_config;
  DescriptorSet*          m_pSet;
  DescriptorSetLayout*    m_pLayout;
  Buffer*                 m_pBuffer;
};
} // Recluse