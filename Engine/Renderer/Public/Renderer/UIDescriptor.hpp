// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Image.hpp"

#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Vector4.hpp"

namespace Recluse {


class DescriptorSet;
class VulkanRHI;
class Texture2D;

struct UITransform {
  Matrix4 _model;
};


class UIDescriptor {
public:


  void            Initialize(VulkanRHI* pRhi);  
  void            CleanUp(VulkanRHI* pRhi);

  // TODO(): For UI Images and textures to be sent to the UI Overlay. May also contain transformation
  // details for it as well..
  UITransform*    GetUIData() { return &m_transform; }

private:

  Texture2D*      m_image;
  UITransform     m_transform;
  DescriptorSet*  m_pSet;
};
} // Recluse