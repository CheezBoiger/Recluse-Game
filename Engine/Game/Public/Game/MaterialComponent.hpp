// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Component.hpp"

#include "Renderer/MaterialDescriptor.hpp"


namespace Recluse {


// A Single instance of a material object stored in gpu memory.
struct Material {
  // Default used if no material is defined by the game object, for the renderer component.
  static Material           _sDefault;
  static void               InitializeDefault() { _sDefault.Initialize(); _sDefault.Native()->Update(); }
  static void               CleanUpDefault() { _sDefault.CleanUp(); }
  static Material*          Default() { return &_sDefault; }
#define MARK_DIRTY_MATERIAL() m_pDesc->SignalUpdate();
  Material() : m_pDesc(nullptr) { }

  void                      Initialize();
  void                      CleanUp();

  r32                       EmissiveFactor() const { return m_pDesc->Data()->_emissiveFactor; }
  r32                       MetallicFactor() const { return m_pDesc->Data()->_metalFactor; }
  r32                       RoughFactor() const { return m_pDesc->Data()->_roughFactor; }

  void                      SetRoughnessFactor(r32 rough) { m_pDesc->Data()->_roughFactor = rough; MARK_DIRTY_MATERIAL(); }
  void                      SetMetallicFactor(r32 metal) { m_pDesc->Data()->_metalFactor = metal; MARK_DIRTY_MATERIAL(); }
  void                      SetEmissiveFactor(r32 emissive) { m_pDesc->Data()->_emissiveFactor = emissive; MARK_DIRTY_MATERIAL(); }

  void                      SetAlbedo(Texture2D* texture) { m_pDesc->SetAlbedo(texture); MARK_DIRTY_MATERIAL(); }
  void                      SetNormal(Texture2D* texture) { m_pDesc->SetNormal(texture); MARK_DIRTY_MATERIAL(); }
  void                      SetRoughnessMetallic(Texture2D* texture) { m_pDesc->SetRoughnessMetallic(texture); MARK_DIRTY_MATERIAL(); }
  void                      SetAo(Texture2D* texture) { m_pDesc->SetAo(texture); MARK_DIRTY_MATERIAL(); }
  void                      SetEmissive(Texture2D* texture) { m_pDesc->SetEmissive(texture); MARK_DIRTY_MATERIAL(); }
  void                      SetBaseColor(Vector4 color) { m_pDesc->Data()->_Color = color; MARK_DIRTY_MATERIAL(); }

  void                      EnableAlbedo(b8 enable) { m_pDesc->Data()->_HasAlbedo = enable; MARK_DIRTY_MATERIAL(); }
  void                      EnableNormal(b8 enable) { m_pDesc->Data()->_HasNormal = enable; MARK_DIRTY_MATERIAL(); }
  void                      EnableRoughness(b8 enable) { m_pDesc->Data()->_HasRoughness = enable; MARK_DIRTY_MATERIAL(); }
  void                      EnableMetallic(b8 enable) { m_pDesc->Data()->_HasMetallic = enable; MARK_DIRTY_MATERIAL(); }
  void                      EnableEmissive(b8 enable) { m_pDesc->Data()->_HasEmissive = enable; MARK_DIRTY_MATERIAL(); }
  void                      EnableAo(b8 enable) { m_pDesc->Data()->_HasAO = enable; MARK_DIRTY_MATERIAL(); }

  // Returns the native descriptor.
  MaterialDescriptor*       Native() { return m_pDesc; }

private:
  MaterialDescriptor*       m_pDesc;
};


class MaterialComponent : public Component {
  RCOMPONENT(MaterialComponent);
public:
  MaterialComponent() : m_pRef(nullptr) { }

  void          SetMaterialRef(Material* ref) { m_pRef = ref; }
  Material*     GetMaterial() { return m_pRef; }


protected:

  void          Update() override { }

private:
  Material*     m_pRef;
};
} // Recluse