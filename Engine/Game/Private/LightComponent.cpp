// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "LightComponent.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"

#include "Core/Math/Vector4.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/LightDescriptor.hpp"
#include "PointLightComponent.hpp"


namespace Recluse {


std::queue<u32> PointLightComponent::sAvailablePointLightIds;
std::queue<u32> LightComponent::sAvailableDirectionalLightIds;


void LightComponent::Initialize()
{
  u32 pointLightCount = LightBuffer::MaxNumPointLights();
  u32 dirLightCount = LightBuffer::MaxNumDirectionalLights();

  for (u32 i = 0; i < pointLightCount; ++i) {
    PointLightComponent::sAvailablePointLightIds.push(i);
  }

  for (u32 i = 0; i < dirLightCount; ++i) {
    sAvailableDirectionalLightIds.push(i);
  }
}


void LightComponent::CleanUp()
{
  // uhh? what to clean up? Renderer takes care of this shat.
}


void PointLightComponent::OnInitialize(GameObject* owner)
{
  if (sAvailablePointLightIds.empty()) {
    R_DEBUG(rError, "Point light can not be assigned a point light! Max exceeded!\n");
    return;
  }
  m_Id = sAvailablePointLightIds.front();
  sAvailablePointLightIds.pop();

  LightBuffer* lights = gRenderer().LightData();
  m_NativeLight = &lights->_PointLights[m_Id];
  m_NativeLight->_Enable = true;
  m_NativeLight->_Range = 1.0f;
  m_NativeLight->_Intensity = 1.0f;
  m_NativeLight->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
}


void PointLightComponent::OnCleanUp()
{
  if (m_Id >= LightBuffer::MaxNumPointLights()) {
    return;
  }
  
  // Push back light id as it is now available.
  sAvailablePointLightIds.push(m_Id);
  m_NativeLight->_Enable = false;
}


void PointLightComponent::Update()
{
  Transform* transform = GetOwner()->GetTransform();
  if (!transform) {
    return;
  }

  m_NativeLight->_Position = transform->Position;
}
} // Recluse