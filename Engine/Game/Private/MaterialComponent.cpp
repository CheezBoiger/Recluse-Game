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
  m_pDesc->Initialize();
  m_pDesc->PushUpdate(MaterialDescriptor::MATERIAL_BUFFER_UPDATE | MaterialDescriptor::MATERIAL_BUFFER_UPDATE);
}


void Material::CleanUp()
{
  gRenderer().FreeMaterialDescriptor(m_pDesc);
  m_pDesc = nullptr;
}


Material Material::_sDefault;


} // Recluse