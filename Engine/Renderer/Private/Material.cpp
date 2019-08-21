// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "Material.hpp"
#include "MeshData.hpp"
#include "Renderer.hpp"


namespace Recluse {


void  Material::initializeDefault(Renderer* pRenderer) 
{
  _sDefault.initialize(pRenderer);
  _sDefault.getNative()->pushUpdate(MATERIAL_DESCRIPTOR_UPDATE_BIT | MATERIAL_BUFFER_UPDATE_BIT);
  _sDefault.setBaseColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
  _sDefault.getNative()->update(gRenderer().getRHI());
}


void Material::initialize(Renderer* pRenderer)
{
  m_pDesc = pRenderer->createMaterialDescriptor();
  m_pDesc->initialize(gRenderer().getRHI());
  m_pDesc->pushUpdate(MATERIAL_BUFFER_UPDATE_BIT | MATERIAL_DESCRIPTOR_UPDATE_BIT);
}


void Material::cleanUp(Renderer* pRenderer)
{
  pRenderer->freeMaterialDescriptor(m_pDesc);
  m_pDesc = nullptr;
}


void Material::enableMaps(B32 bits)
{
  if (bits & MAT_ALBEDO_BIT) enableAlbedo(true);
  if (bits & MAT_NORMAL_BIT) enableNormal(true);
  if (bits & MAT_ROUGH_BIT) enableRoughness(true);
  if (bits & MAT_METAL_BIT) enableMetallic(true);
  if (bits & MAT_EMIT_BIT) enableEmissive(true);
  if (bits & MAT_AO_BIT) enableAo(true);
}


void Material::disableMaps(B32 bits)
{
  if (bits & MAT_ALBEDO_BIT) enableAlbedo(false);
  if (bits & MAT_NORMAL_BIT) enableNormal(false);
  if (bits & MAT_ROUGH_BIT) enableRoughness(false);
  if (bits & MAT_METAL_BIT) enableMetallic(false);
  if (bits & MAT_EMIT_BIT) enableEmissive(false);
  if (bits & MAT_AO_BIT) enableAo(false);
}


Material Material::_sDefault;


} // Recluse