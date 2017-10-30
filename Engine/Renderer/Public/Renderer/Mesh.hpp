// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"


namespace Recluse {


class VulkanRHI;

// TODO(): Structure this object to generate submeshes when needed.
class Mesh {
public:
  Mesh()
    : mVisible(true)
    , mRenderable(true)
    , mTransparent(false)
    , mTranslucent(false)
    , mStatic(true) { }

  void          Initialize(size_t elementCount, size_t sizeType, void* data, 
                  b8 isStatic, size_t indexCount = 0, void* indices = nullptr);
  void          CleanUp();

  VertexBuffer* GetVertexBuffer() { return &mVertexBuffer; }
  IndexBuffer*  GetIndexBuffer() { return &mIndexBuffer; }

  void          SetVisible(b8 enable) { mVisible = enable; }
  void          SetRenderable(b8 enable) { mRenderable = enable; }
  void          SetTransparent(b8 enable) { mTransparent = enable; }
  void          SetTranslucent(b8 enable) { mTranslucent = enable; }

  b8            Visible() const { return mVisible; }
  b8            Renderable() const { return mRenderable; }
  b8            Transparent() const { return mTransparent; }
  b8            Translucent() const { return mTranslucent; }
  b8            Static() const { return mStatic; }

private:
  VertexBuffer  mVertexBuffer;
  IndexBuffer   mIndexBuffer;

  b8            mVisible;
  b8            mRenderable;

  b8            mTransparent;
  b8            mTranslucent;
  b8            mStatic;
  
  VulkanRHI*    mRhi;
  
  friend class Renderer;
};
} // Recluse