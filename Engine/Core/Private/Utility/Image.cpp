// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Utility/Image.hpp"
#include "Utility/stb_image.hpp"
#include "stb_image_write.hpp"

namespace Recluse {


Image::~Image()
{
}


B8 Image::load(const TChar* imgpath)
{
  _data = stbi_load(imgpath, &_width, &_height, &_channels, STBI_rgb_alpha);
  if (!_data) return false;

  _memorySize = _width * _height * 4;
  return true;
}


B8 Image::SavePNG(const TChar* imgpath)
{
  if (!_data) { return false; }

  stbi_write_png(imgpath, _width, _height, _channels, _data, _width * _channels);
  return true;
}


B8 Image::SaveHDR(const TChar* imgPath)
{
  if (!_data) { return false; }
  stbi_write_hdr(imgPath, _width, _height, _channels, (const R32*)_data);
  return true;
}


B8 Image::cleanUp()
{
  if (_data) {
    stbi_image_free(_data);
    _data = nullptr;
    return false;
  }
  return false;
}
} // Recluse