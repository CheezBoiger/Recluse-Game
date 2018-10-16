// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Scene/ModelLoader.hpp"
#include "ModelLoaderGLTF.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"

#if INCLUDE_FBX
#include "ModelLoaderFBX.hpp"
#endif

namespace Recluse {

static const u32 kMaxFbxExtensions = 1;
static const u32 kMaxGLTFExtensions = 2;

const char* allowed_fbx_extensions[kMaxFbxExtensions] = {
  "fbx"
};


const char* allowed_gltf_extensions[kMaxGLTFExtensions] = {
  "gltf",
  "glb"
};


enum FileType {
  FILETYPE_UNKNOWN = -1,
  FILETYPE_FBX,
  FILETYPE_GLTF
};


namespace ModelLoader {


std::string GetFilenameExt(const std::string& path)
{
  size_t cutoff = path.find_last_of('/');
  if (cutoff != std::string::npos) {
    size_t removeExtId = path.find_last_of('.') + 1;
    if (removeExtId != std::string::npos) {
      std::string ext = path.substr(removeExtId, path.size());
      return ext;
    }
  }
}


ModelResultBits Load(const std::string filename)
{
  FileType type = FILETYPE_UNKNOWN;
  std::string ext = GetFilenameExt(filename);

  for (u32 i = 0; i < kMaxGLTFExtensions; ++i) {
    if (strcmp(allowed_gltf_extensions[i], ext.c_str()) == 0) {
      type = FILETYPE_GLTF;
      R_DEBUG(rNotify, "parsing GLTF file.\n");
      break;
    }
  }

  if (type == FILETYPE_UNKNOWN) {
    for (u32 i = 0; i < kMaxFbxExtensions; ++i) {
      if (strcmp(allowed_fbx_extensions[i], ext.c_str()) == 0) {
        type = FILETYPE_FBX;
        R_DEBUG(rNotify, "parsing FBX file.\n");
        if (!INCLUDE_FBX) {
          R_DEBUG(rWarning, "This build does not include fbx loader! Be sure to add -DUSE_FBX=ON in your cmake build!\n");
        }
        break;
      }
    }
  }

  ModelResultBits result = 0;
  switch (type) {
#if INCLUDE_FBX
    case FILETYPE_FBX:
      result = FBX::Load(filename);
      break;
#endif
    case FILETYPE_GLTF:
    default:
      result = GLTF::Load(filename);
  }
  return result;
}
} // ModelLoader
} // Recluse