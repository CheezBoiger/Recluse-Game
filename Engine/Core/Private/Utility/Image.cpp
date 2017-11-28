// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Utility/Image.hpp"

#include "stb_image.hpp"

namespace Recluse {


Image::~Image()
{
  CleanUp();
}


b8 Image::Load(const tchar* imgpath)
{
  mData = stbi_load(imgpath, &mWidth, &mHeight, &mChannels, STBI_rgb_alpha);
  if (!mData) return false;

  mMemorySize = mWidth * mHeight * 4;
  return true;
}


b8 Image::CleanUp()
{
  if (mData) {
    stbi_image_free(mData);
    mData = nullptr;
    return false;
  }
  return false;
}
} // Recluse