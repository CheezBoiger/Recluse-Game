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
class Sampler;
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
  UIDescriptor()
    : m_pSet(nullptr)
    , m_image(nullptr)
    , m_sampler(nullptr)
    , m_needsUpdate(false) { }

  void            initialize(VulkanRHI* pRhi);  
  void            update(VulkanRHI* pRhi);
  void            cleanUp(VulkanRHI* pRhi);
  void            SetImage(Texture2D* pTex) { m_image = pTex; MarkToUpdate(); }
  Texture2D*        GetImage() { return m_image; }
  
  DescriptorSet*    getDescriptorSet() { return m_pSet; }

private:

  void            MarkToUpdate() { m_needsUpdate = true; }

  Texture2D*        m_image;
  Sampler*        m_sampler;
  DescriptorSet*    m_pSet;
  UIDescriptorType  m_type;
  B32               m_needsUpdate;
};
} // Recluse