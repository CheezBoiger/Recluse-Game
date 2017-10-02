// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"


namespace Recluse {


class Mesh {
public:
  Mesh() { }

  void          Initialize(size_t elementCount, void* data, 
                  size_t indexCount = 0, void* indices = nullptr);
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

private:
  VertexBuffer  mVertexBuffer;
  IndexBuffer   mIndexBuffer;

  b8            mVisible;
  b8            mRenderable;

  b8            mTransparent;
  b8            mTranslucent;
};
} // Recluse