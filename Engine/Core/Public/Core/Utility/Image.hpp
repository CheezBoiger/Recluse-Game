// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


// Image constitutes a displayable representation of a textured colored
// surface. This is the very representation of some image.

// TODO(): Need to accomodate textured cubes.
class Image {
public:
  Image()
    : mWidth(0), mHeight(0), mChannels(0), mData(nullptr), mMemorySize(0) { }

  ~Image();

  b8            Load(const tchar* imgpath);

  // Clean up image data. Returns true if image was cleaned up. Returns false if
  // no image data was cleaned up (which could mean there was no data to begin with.)
  b8            CleanUp();
  b8            ContainsData() const { if (mData) return true; return false; }

  i32           Height() const { return mHeight; }
  i32           Width() const { return mWidth; }
  const u8*     Data() const { return mData; }
  u64           MemorySize() const { return mMemorySize; }         

private:
  u64           mMemorySize;
  i32           mWidth;
  i32           mHeight;
  i32           mChannels;
  u8*           mData;
};
} // Recluse