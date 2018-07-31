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

  r32 _bloomStrength;
};


// Realtime configurations of hdr settings.
struct ConfigHDR {
  Vector4 _allowChromaticAberration;
  Vector4 _k;
  Vector4 _kcube;
  Vector4 _interleavedVideo;
};


class HDR {
public:
  HDR();

  ~HDR();

  void                    Initialize(VulkanRHI* pRhi);
  void                    CleanUp(VulkanRHI* pRhi);

  void                    UpdateToGPU(VulkanRHI* pRhi);

  ConfigHDR*              GetRealtimeConfiguration() { return &m_config; }
  DescriptorSet*          GetSet() const { return m_pSet; };
  DescriptorSetLayout*    GetSetLayout() const { return m_pLayout; }

private:

  ConfigHDR               m_config;
  DescriptorSet*          m_pSet;
  DescriptorSetLayout*    m_pLayout;
  Buffer*                 m_pBuffer;
};
} // Recluse