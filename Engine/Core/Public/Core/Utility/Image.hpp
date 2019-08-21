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

  B8            load(const TChar* imgpath);
  B8            SaveHDR(const TChar* imgpath);
  B8            SavePNG(const TChar* imgpath);

  // Clean up image data. Returns true if image was cleaned up. Returns false if
  // no image data was cleaned up (which could mean there was no data to begin with.)
  B8            cleanUp();
  B8            ContainsData() const { if (_data) return true; return false; }

  I32           getHeight() const { return _height; }
  I32           getWidth() const { return _width; }
  const U8*     getData() const { return _data; }
  U64           MemorySize() const { return _memorySize; }         

  U64           _memorySize;
  I32           _width;
  I32           _height;
  I32           _channels;
  U8*           _data;
};
} // Recluse