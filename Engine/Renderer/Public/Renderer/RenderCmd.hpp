// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Color4.hpp"
#include <vector>

namespace Recluse {


class MaterialDescriptor;
class MeshDescriptor;
class RenderMaterial;
class JointDescriptor;
class MeshData;
class Texture2D;

struct Primitive;


enum CmdConfig {
  CMD_RENDERABLE_BIT  = (1 << 0),
  CMD_DEBUG_BIT       = (1 << 2),
  CMD_SHADOWS_BIT     = (1 << 3),
  CMD_INSTANCING_BIT  = (1 << 4),
  CMD_TRANSLUCENT_BIT = (1 << 5),
  CMD_TRANSPARENT_BIT = (1 << 6),
  CMD_FORWARD_BIT     = (1 << 7),
  CMD_ALLOW_CULL_BIT  = (1 << 8),
  CMD_DECAL_BIT       = (1 << 9)
};


enum UiType {
  UI_TEXT,
  UI_BEGIN,
  UI_END
};

  
typedef u32 CmdConfigBits;


struct MeshRenderCmd {
  MeshRenderCmd()
    : _config(false)
    , _pMeshDesc(nullptr)
    , _pMeshData(nullptr)
    , _pPrimitives(nullptr)
    , _bSkinned(false)
    , _instances(1) { }

  MeshData*               _pMeshData;
  MeshDescriptor*         _pMeshDesc;
  JointDescriptor*        _pJointDesc;
  Primitive*              _pPrimitives;
  u32                     _primitiveCount;
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


class BufferUI {
public:
  BufferUI(u32 id)
    : m_id(id) { }

  void Initialize();
  void BeginCanvas(const UiBeginCanvasInfo& begin);
  void EndCanvas();
  void EmitText(const UiText& text);

  u32   GetId() const { return m_id; }

private:
  u32 m_id;
};
} // Recluse