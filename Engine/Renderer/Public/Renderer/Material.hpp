// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"

#include "MaterialDescriptor.hpp"

namespace Recluse {

class Renderer;

enum MaterialMaps {
  MAT_ALBEDO_BIT = (1 << 0),
  MAT_NORMAL_BIT = (1 << 1),
  MAT_ROUGH_BIT = (1 << 2),
  MAT_METAL_BIT = (1 << 3),
  MAT_EMIT_BIT = (1 << 4),
  MAT_AO_BIT = (1 << 5)
};


// A Single instance of a material object stored in gpu memory.
class Material {
public:
  // Default used if no material is defined by the game object, for the renderer component.
  static Material           _sDefault;
  static void               InitializeDefault(Renderer* pRenderer);

  static void               CleanUpDefault(Renderer* pRenderer) { _sDefault.CleanUp(pRenderer); }
  static Material*          Default() { return &_sDefault; }
#define MARK_DIRTY_MATERIAL(b) m_pDesc->PushUpdate(b);
  Material() : m_pDesc(nullptr) { }

  void                      Initialize(Renderer* pRenderer);
  void                      CleanUp(Renderer* pRenderer);

  r32                       EmissiveFactor() const { return m_pDesc->Data()->_emissiveFactor; }
  r32                       MetallicFactor() const { return m_pDesc->Data()->_metalFactor; }
  r32                       RoughFactor() const { return m_pDesc->Data()->_roughFactor; }

  void                      SetRoughnessFactor(r32 rough) { m_pDesc->Data()->_roughFactor = rough; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      SetMetallicFactor(r32 metal) { m_pDesc->Data()->_metalFactor = metal; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      SetEmissiveFactor(r32 emissive) { m_pDesc->Data()->_emissiveFactor = emissive; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      SetOpacity(r32 opaque) { m_pDesc->Data()->_Opacity = opaque; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }

  void                      EnableMaps(b32 mapEnable);
  void                      DisableMaps(b32 mapsDisable);
  void                      SetAlbedoSampler(TextureSampler* sampler) { m_pDesc->SetAlbedoSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetNormalSampler(TextureSampler* sampler) { m_pDesc->SetNormalSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetRoughMetalSampler(TextureSampler* sampler) { m_pDesc->SetRoughMetalSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetAoSampler(TextureSampler* sampler) { m_pDesc->SetAoSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetEmissiveSampler(TextureSampler* sampler) { m_pDesc->SetEmissiveSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetAlbedo(Texture2D* texture) { m_pDesc->SetAlbedo(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetNormal(Texture2D* texture) { m_pDesc->SetNormal(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetRoughnessMetallic(Texture2D* texture) { m_pDesc->SetRoughnessMetallic(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetAo(Texture2D* texture) { m_pDesc->SetAo(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetEmissive(Texture2D* texture) { m_pDesc->SetEmissive(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetBaseColor(Vector4 color) { m_pDesc->Data()->_Color = color; MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      SetTransparent(b32 enable) { m_pDesc->Data()->_IsTransparent = enable; }
  void                      EnableAlbedo(b32 enable) { m_pDesc->Data()->_HasAlbedo = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      EnableNormal(b32 enable) { m_pDesc->Data()->_HasNormal = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      EnableRoughness(b32 enable) { m_pDesc->Data()->_HasRoughness = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      EnableMetallic(b32 enable) { m_pDesc->Data()->_HasMetallic = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      EnableEmissive(b32 enable) { m_pDesc->Data()->_HasEmissive = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      EnableAo(b32 enable) { m_pDesc->Data()->_HasAO = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      SetUvOffsets(Vector4 offsets) { m_pDesc->Data()->_offsetUV = offsets; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  // Returns the native descriptor.
  MaterialDescriptor*       Native() { return m_pDesc; }

private:
  MaterialDescriptor*       m_pDesc;
};
} // Recluse