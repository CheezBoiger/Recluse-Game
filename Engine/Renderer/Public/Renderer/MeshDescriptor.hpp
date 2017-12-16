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
  Matrix4 model;
  Matrix4 normalMatrix;
  Vector4 color;
  r32     lodBias;
  r32     transparency;
  r32     baseMetal;
  r32     baseRough;
  u32     hasAlbedo;
  u32     hasMetallic;
  u32     hasRoughness;
  u32     hasNormal;
  u32     hasEmissive;
  u32     hasAO;
  u32     hasBones;
  u32     isTransparent;
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
  Buffer*       NativeObjectBuffer() { return mObjectBuffer; }

protected:
  ObjectBuffer  mObjectData;
  Buffer*       mObjectBuffer;

  b8            mVisible;
  b8            mRenderable;

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