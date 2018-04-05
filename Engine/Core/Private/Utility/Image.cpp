// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Utility/Image.hpp"
#include "Utility/stb_image.hpp"

namespace Recluse {


Image::~Image()
{
  CleanUp();
}


b8 Image::Load(const tchar* imgpath)
{
  _data = stbi_load(imgpath, &_width, &_height, &_channels, STBI_rgb_alpha);
  if (!_data) return false;

  _memorySize = _width * _height * 4;
  return true;
}


b8 Image::CleanUp()
{
  if (_data) {
    stbi_image_free(_data);
    _data = nullptr;
    return false;
  }
  return false;
}
} // Recluse