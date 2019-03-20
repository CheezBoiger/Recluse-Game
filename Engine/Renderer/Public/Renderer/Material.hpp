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
  static void               initializeDefault(Renderer* pRenderer);

  static void               cleanUpDefault(Renderer* pRenderer) { _sDefault.cleanUp(pRenderer); }
  static Material*          getDefault() { return &_sDefault; }
#define MARK_DIRTY_MATERIAL(b) m_pDesc->pushUpdate(b);
  Material() : m_pDesc(nullptr) { }

  void                      initialize(Renderer* pRenderer);
  void                      cleanUp(Renderer* pRenderer);

  r32                       emissiveFactor() const { return m_pDesc->getData()->_emissiveFactor; }
  r32                       metallicFactor() const { return m_pDesc->getData()->_metalFactor; }
  r32                       roughFactor() const { return m_pDesc->getData()->_roughFactor; }

  void                      setRoughnessFactor(r32 rough) { m_pDesc->getData()->_roughFactor = rough; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      setMetallicFactor(r32 metal) { m_pDesc->getData()->_metalFactor = metal; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      setEmissiveFactor(r32 emissive) { m_pDesc->getData()->_emissiveFactor = emissive; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      setOpacity(r32 opaque) { m_pDesc->getData()->_Opacity = opaque; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }

  void                      enableMaps(b32 mapEnable);
  void                      disableMaps(b32 mapsDisable);
  void                      setAlbedoSampler(TextureSampler* sampler) { m_pDesc->setAlbedoSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setNormalSampler(TextureSampler* sampler) { m_pDesc->setNormalSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setRoughMetalSampler(TextureSampler* sampler) { m_pDesc->setRoughMetalSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setAoSampler(TextureSampler* sampler) { m_pDesc->setAoSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setEmissiveSampler(TextureSampler* sampler) { m_pDesc->setEmissiveSampler(sampler); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setAlbedo(Texture2D* texture) { m_pDesc->setAlbedo(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setNormal(Texture2D* texture) { m_pDesc->setNormal(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setRoughnessMetallic(Texture2D* texture) { m_pDesc->setRoughnessMetallic(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setAo(Texture2D* texture) { m_pDesc->setAo(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setEmissive(Texture2D* texture) { m_pDesc->setEmissive(texture); MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setBaseColor(Vector4 color) { m_pDesc->getData()->_Color = color; MARK_DIRTY_MATERIAL(MATERIAL_DESCRIPTOR_UPDATE_BIT); }
  void                      setTransparent(b32 enable) { m_pDesc->getData()->_IsTransparent = enable; }
  void                      enableAlbedo(b32 enable) { m_pDesc->getData()->_HasAlbedo = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      enableNormal(b32 enable) { m_pDesc->getData()->_HasNormal = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      enableRoughness(b32 enable) { m_pDesc->getData()->_HasRoughness = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      enableMetallic(b32 enable) { m_pDesc->getData()->_HasMetallic = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      enableEmissive(b32 enable) { m_pDesc->getData()->_HasEmissive = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      enableAo(b32 enable) { m_pDesc->getData()->_HasAO = enable; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  void                      setUvOffsets(Vector4 offsets) { m_pDesc->getData()->_offsetUV = offsets; MARK_DIRTY_MATERIAL(MATERIAL_BUFFER_UPDATE_BIT); }
  // Returns the native descriptor.
  MaterialDescriptor*       getNative() { return m_pDesc; }

private:
  MaterialDescriptor*       m_pDesc;
};
} // Recluse