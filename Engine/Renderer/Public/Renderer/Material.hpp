// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"

namespace Recluse {


class VulkanRHI;
class DescriptorSet;
class Buffer;

// TODO(): Needs to hold uniform buffers, samplers, and images.
class Material {
public:

  void            SetGlobalBufferRef(DescriptorSet* glob) { mGlobalBufferSetRef = glob; }
  void            SetLightBufferRef(DescriptorSet* light) { mLightBufferSetRef = light; }

  DescriptorSet*  ObjectBufferSet() { return mObjectBufferSetRef; }
  DescriptorSet*  GlobalBufferSet() { return mGlobalBufferSetRef; }
  DescriptorSet*  LightBufferSet() { return mLightBufferSetRef; }

  void            Update();

private:

  DescriptorSet*  mGlobalBufferSetRef;
  DescriptorSet*  mObjectBufferSetRef;
  DescriptorSet*  mLightBufferSetRef;
};
} // Recluse