// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Math/Matrix4.hpp"

namespace Recluse {

class Buffer;
class Renderer;
class MeshData;


struct ObjectBuffer {
  Matrix4 model;          // Model matrix
  Matrix4 normalMatrix;   // Normal matrix.
  Vector4 color;          // object base color.
  r32     lodBias;        // object level of detail bias.
  r32     transparency;   // transparency [0.0, 1.0]
  r32     baseMetal;      // object base metalness [0.0, 1.0]
  r32     baseRough;      // object base roughness [0.0, 1.0]
  r32     baseEmissive;   // emissive base [0.0, inf]
  u32     hasAlbedo;      // does object have albedo map?
  u32     hasMetallic;    // does object have metalness map?
  u32     hasRoughness;   // does object have roughness map?
  u32     hasNormal;      // does object have normal map?
  u32     hasEmissive;    // does object have emissive map?
  u32     hasAO;          // does object have ambient occlusion map?
  u32     hasBones;       // does object have bones?
  u32     isTransparent;  // is object transparent?
  u32     pad[3];
};


struct BonesBuffer {
  Matrix4 bones[64];
};


// MeshDesciptor is a descriptor that defines how to render an object. This is needed in order
// to show something on display, as the renderer relies heavily on this object for
// command buffer creation. It basically defines the transform of the meshdata to render, as well
// as whether that meshdata is defined as transparent, translucent, visible, and/or renderable.
class MeshDescriptor {
public:
  MeshDescriptor();
  virtual ~MeshDescriptor();

  virtual void  Initialize(Renderer* renderer);
  virtual void  CleanUp();

  virtual void  Update();

  void          SetVisible(b8 enable) { mVisible = enable; }
  void          SetRenderable(b8 enable) { mRenderable = enable; }
  void          SetTranslucent(b8 enable) { mTranslucent = enable; }
  void          SetTransparent(b8 enable) { mObjectData.isTransparent = enable; }

  ObjectBuffer* ObjectData() { return &mObjectData; }


  b8            Visible() const { return mVisible; }
  b8            Renderable() const { return mRenderable; }
  b8            Transparent() const { return mObjectData.isTransparent; }
  b8            Translucent() const { return mTranslucent; }
  b8            Static() const { return mStatic; }
  b8            Skinned() const { return mSkinned; }
  Buffer*       NativeObjectBuffer() { return mObjectBuffer; }

protected:
  ObjectBuffer  mObjectData;
  Buffer*       mObjectBuffer;

  b8            mVisible;
  b8            mRenderable;
  b8            mSkinned;

  b8            mTranslucent;
  b8            mStatic;
  
  Renderer*     mRenderer;
  
  friend class Renderer;
};


class SkinnedMeshDescriptor : public MeshDescriptor {
public:
  SkinnedMeshDescriptor();
  virtual ~SkinnedMeshDescriptor();
  
  virtual void  Initialize(Renderer* renderer) override;
  virtual void  CleanUp() override;
  virtual void  Update() override;

  BonesBuffer*  BonesData() { return &mBonesData; }
  Buffer*       NativeBoneBuffer() { return mBonesBuffer; }

private:
  BonesBuffer   mBonesData;
  Buffer*       mBonesBuffer;
  friend class  Renderer;
};
} // Recluse