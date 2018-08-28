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
class Texture2D;
class UIDescriptor;

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
  CMD_STATIC_BIT        = (1 << 11)   // Mesh is static, or is not changing. Useful for shadow maps.
};


enum UiType {
  UI_TEXT,
  UI_IMAGE,
  UI_BEGIN,
  UI_END
};

  
typedef u32 CmdConfigBits;


struct MeshRenderCmd {
  MeshRenderCmd()
    : _config(false)
    , _pMeshDesc(nullptr)
    , _pMeshData(nullptr)
    , _bSkinned(false)
    , _instances(1) { }

  MeshData*               _pMeshData;
  MeshDescriptor*         _pMeshDesc;
  JointDescriptor*        _pJointDesc;
  u32                     _instances;
  b32                     _bSkinned;
  CmdConfigBits           _config;
};


struct DecalRenderCmd {
  DecalRenderCmd()
    : _pTexture(nullptr)
      { }

  Texture2D*  _pTexture;
  Matrix4     _transform;
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

  void Initialize();
  void BeginCanvas(const UiBeginCanvasInfo& begin);
  void EndCanvas();
  void EmitText(const UiText& text);
  void EmitImage(const UiImageInfo& imgInfo);

  u32   GetId() const { return m_id; }

private:
  u32 m_id;
};
} // Recluse