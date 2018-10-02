// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


// Image constitutes a displayable representation of a textured colored
// surface. This is the very representation of some image.

// TODO(): Need to accomodate textured cubes.
struct Image {
  Image()
    : _width(0), _height(0), _channels(0), _data(nullptr), _memorySize(0) { }

  ~Image();

  b8            Load(const tchar* imgpath);
  b8            SaveHDR(const tchar* imgpath);
  b8            SavePNG(const tchar* imgpath);

  // Clean up image data. Returns true if image was cleaned up. Returns false if
  // no image data was cleaned up (which could mean there was no data to begin with.)
  b8            CleanUp();
  b8            ContainsData() const { if (_data) return true; return false; }

  i32           Height() const { return _height; }
  i32           Width() const { return _width; }
  const u8*     Data() const { return _data; }
  u64           MemorySize() const { return _memorySize; }         

  u64           _memorySize;
  i32           _width;
  i32           _height;
  i32           _channels;
  u8*           _data;
};
} // Recluse