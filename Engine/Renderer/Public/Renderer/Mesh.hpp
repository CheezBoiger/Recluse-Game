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
  r32     lodBias;
  u32     hasAlbedo;
  u32     hasMetallic;
  u32     hasRoughness;
  u32     hasNormal;
  u32     hasEmissive;
  u32     hasAO;
  u32     hasBones;
};


struct BonesBuffer {
  Matrix4 bones[64];
};

// TODO(): Structure this object to generate submeshes when needed.
// Mesh is an object that defines how to render an object. This is needed in order
// to show something on display, as the renderer relies heavily on this object for
// command buffer creation. 
class Mesh {
public:
  Mesh();
  virtual ~Mesh() { }

  virtual void  Initialize(Renderer* renderer);
  virtual void  CleanUp();
  
  MeshData*     Data() { return mMeshData; }

  virtual void  Update();

  void          SetMeshData(MeshData* meshData) { mMeshData = meshData; }
  void          SetVisible(b8 enable) { mVisible = enable; }
  void          SetRenderable(b8 enable) { mRenderable = enable; }
  void          SetTransparent(b8 enable) { mTransparent = enable; }
  void          SetTranslucent(b8 enable) { mTranslucent = enable; }

  ObjectBuffer* ObjectData() { return &mObjectData; }


  b8            Visible() const { return mVisible; }
  b8            Renderable() const { return mRenderable; }
  b8            Transparent() const { return mTransparent; }
  b8            Translucent() const { return mTranslucent; }
  b8            Static() const { return mStatic; }

  Buffer*       NativeObjectBuffer() { return mObjectBuffer; }

protected:
  ObjectBuffer  mObjectData;

  MeshData*     mMeshData;
  Buffer*       mObjectBuffer;

  b8            mVisible;
  b8            mRenderable;

  b8            mTransparent;
  b8            mTranslucent;
  b8            mStatic;
  
  Renderer*     mRenderer;
  
  friend class Renderer;
};


class SkinnedMesh : public Mesh {
public:
  SkinnedMesh();
  
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