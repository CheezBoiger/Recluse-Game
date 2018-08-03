// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "MaterialComponent.hpp"
#include "MeshComponent.hpp"
#include "GameObject.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Renderer.hpp"


namespace Recluse {


void Material::Initialize()
{
  m_pDesc = gRenderer().CreateMaterialDescriptor();
  m_pDesc->Initialize(gRenderer().RHI());
  m_pDesc->PushUpdate(MATERIAL_BUFFER_UPDATE_BIT | MATERIAL_DESCRIPTOR_UPDATE_BIT);
}


void Material::CleanUp()
{
  gRenderer().FreeMaterialDescriptor(m_pDesc);
  m_pDesc = nullptr;
}


void Material::EnableMaps(b32 bits)
{
  if (bits & MAT_ALBEDO_BIT) EnableAlbedo(true);
  if (bits & MAT_NORMAL_BIT) EnableNormal(true);
  if (bits & MAT_ROUGH_BIT) EnableRoughness(true);
  if (bits & MAT_METAL_BIT) EnableMetallic(true);
  if (bits & MAT_EMIT_BIT) EnableEmissive(true);
  if (bits & MAT_AO_BIT) EnableAo(true);
}


void Material::DisableMaps(b32 bits)
{
  if (bits & MAT_ALBEDO_BIT) EnableAlbedo(false);
  if (bits & MAT_NORMAL_BIT) EnableNormal(false);
  if (bits & MAT_ROUGH_BIT) EnableRoughness(false);
  if (bits & MAT_METAL_BIT) EnableMetallic(false);
  if (bits & MAT_EMIT_BIT) EnableEmissive(false);
  if (bits & MAT_AO_BIT) EnableAo(false);
}


Material Material::_sDefault;


} // Recluse