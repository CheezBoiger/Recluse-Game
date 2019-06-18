// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Color4.hpp"
#include <vector>

#include "MeshData.hpp"

namespace Recluse {


class MaterialDescriptor;
class MeshDescriptor;
class RenderMaterial;
class JointDescriptor;
class MeshData;
class MorphTarget;
class Texture2D;
class UIDescriptor;
class Material;

struct Primitive;


enum CmdConfig {
  CMD_RENDERABLE_BIT    = (1 << 0),   // Allows the mesh to be renderable.
  CMD_DEBUG_BIT         = (1 << 2),   // Allows mesh to be debuggable.
  CMD_SHADOWS_BIT       = (1 << 3),   // Mesh allowed to cast shadows.
  CMD_INSTANCING_BIT    = (1 << 4),   // Mesh is instanced.
  CMD_TRANSLUCENT_BIT   = (1 << 5),   // Mesh is translucent.
  CMD_TRANSPARENT_BIT   = (1 << 6),   // Mesh is transparent.
  CMD_FORWARD_BIT       = (1 << 7),   // Mesh will be pushed to the forward render pipeline.
  CMD_ALLOW_CULL_BIT    = (1 << 8),   // Mesh can be culled.
  CMD_DECAL_BIT         = (1 << 9),   // Mesh is a decal.
  CMD_WATER_REFLECT_BIT = (1 << 10),  // Allow mesh to be reflected by water.
  CMD_STATIC_BIT        = (1 << 11),  // Mesh is static, or is not changing. Useful for shadow maps.
  CMD_SKINNED_BIT       = (1 << 12),  // command is skinned, contains joint buffer.
  CMD_MORPH_BIT         = (1 << 13),  // command contains morph targets.
  CMD_WIREFRAME_BIT     = (1 << 14),  // Wireframe mode.
  CMD_BASIC_RENDER_BIT  = (1 << 15),  // Basic render bit, redirects rendering to simple pipeline, with no material.
  CMD_REFRACTION_BIT    = (1 << 16),  // Refraction bit. Mesh refracts.
  CMD_CUSTOM_SHADE_BIT  = (1 << 17)   // Mesh uses custom shader.
};


enum DebugCmdConfig {
  DEBUG_CONFIG_NONE_BIT = 0,
  DEBUG_CONFIG_ALBEDO_BIT = (1 << 0),
  DEBUG_CONFIG_NORMAL_BIT = (1 << 1),
  DEBUG_CONFIG_ROUGH_BIT = (1 << 2),
  DEBUG_CONFIG_METAL_BIT = (1 << 3),
  DEBUG_CONFIG_EMISSIVE_BIT = (1 << 4),
  DEBUG_CONFIG_DIRECT_LIGHT_BIT = (1 << 5),
  DEBUG_CONFIG_POINT_LIGHT_BIT = (1 << 6),
  DEBUG_CONFIG_IBL_BIT = (1 << 7),
  DEBUG_CONFIG_WIREFRAME = (1 << 8)
};


enum UiType {
  UI_TEXT,
  UI_IMAGE,
  UI_BEGIN,
  UI_END
};

  
typedef u32 CmdConfigBits;
typedef u32 DebugConfigBits;


struct MeshRenderCmd {
  MeshRenderCmd()
    : _pMeshDesc(nullptr)
    , _pMeshData(nullptr)
    , _pMorph0(nullptr)
    , _pMorph1(nullptr)
    , _debugConfig(0)
    , _instances(1) { }

  MeshData*               _pMeshData;
  Primitive*              _pPrimitives;
  MorphTarget*            _pMorph0;
  MorphTarget*            _pMorph1;
  MeshDescriptor*         _pMeshDesc;
  JointDescriptor*        _pJointDesc;
  u32                     _instances;
  u32                     _primitiveCount;
  CmdConfigBits           _config;
  DebugConfigBits         _debugConfig;       // isDebug only if CMD_DEBUG_BIT is on.
};


struct PrimitiveRenderCmd {
  MeshData*               _pMeshData;
  Primitive*              _pPrimitive;
  MorphTarget*            _pMorph0;
  MorphTarget*            _pMorph1;
  MeshDescriptor*         _pMeshDesc;
  JointDescriptor*        _pJointDesc;
  u32                     _instances;
  CmdConfigBits           _config;
  DebugConfigBits         _debugConfig;
};


struct BasicDebugRenderCmd : public MeshRenderCmd {
  Vector4                 _color;
};


struct DecalRenderCmd {
  DecalRenderCmd()
    : _pMat(nullptr)
      { }

  Material*   _pMat;
  Matrix4     _obb;
  Matrix4     _scale;
};


struct UiRenderCmd {
  UiType          _uiType;
  CmdConfigBits   _config;
};


struct UiText : public UiRenderCmd {
  UiText() { _uiType = UI_TEXT; }

  r32         _x;
  r32         _y;
  r32         _width;
  r32         _height;
  char        _str[128];
  size_t      _sz;
  Color4      _bgColor;
  Color4      _fgColor;
};



struct UiBeginCanvasInfo : public UiRenderCmd {
  UiBeginCanvasInfo() { _uiType = UI_BEGIN; }
  Color4        _fixedBackgroundColor;
  Color4        _backgroundColor;
  Color4        _canvasBorderColor;
  Color4        _headerColor;
  char          _str[128];
  r32           _x;
  r32           _y;
  r32           _width;
  r32           _height;
};


struct UiImageInfo : public UiRenderCmd {
  UiImageInfo() { _uiType = UI_IMAGE; }

  r32             _x;
  r32             _y;
  r32             _width;
  r32             _height;
  u16             _region[4];
  UIDescriptor*   _descriptor;
};


class BufferUI {
public:
  BufferUI(u32 id)
    : m_id(id) { }

  void initialize();
  void BeginCanvas(const UiBeginCanvasInfo& begin);
  void EndCanvas();
  void EmitText(const UiText& text);
  void EmitImage(const UiImageInfo& imgInfo);

  u32   getId() const { return m_id; }

private:
  u32 m_id;
};
} // Recluse