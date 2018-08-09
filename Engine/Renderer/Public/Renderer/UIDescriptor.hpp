// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Image.hpp"

#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Vector4.hpp"

namespace Recluse {


class DescriptorSet;
class VulkanRHI;
class Buffer;
class Texture2D;

struct UITransform {
  Matrix4 _model;
};


enum UIDescriptorType {
  UI_DESCRIPTOR_TYPE_SPINNER,
  UI_DESCRIPTOR_TYPE_IMAGE,
  UI_DESCRIPTOR_TYPE_TEXT
};


class UIDescriptor {
public:


  void            Initialize(VulkanRHI* pRhi);  
  void            Update(VulkanRHI* pRhi);
  void            CleanUp(VulkanRHI* pRhi);

  Texture2D*        GetImage() { return m_image; }

private:

  Texture2D*        m_image;
  DescriptorSet*    m_pSet;
  UIDescriptorType  m_type;
};
} // Recluse