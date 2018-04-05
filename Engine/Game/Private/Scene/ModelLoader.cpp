// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Scene/ModelLoader.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"

#include "Renderer/Vertex.hpp"

#include "Rendering/TextureCache.hpp"
#include "tiny_gltf.hpp"


namespace Recluse {
namespace ModelLoader {


void LoadTextures(tinygltf::Model* gltfModel, Model* engineModel)
{
  for (tinygltf::Image& image : gltfModel->images) {
    Texture2D* pTex = gRenderer().CreateTexture2D();
    pTex->Initialize(static_cast<u32>(image.width), 
                  static_cast<u32>(image.height));
    Image img;
    img._data = image.image.data();
    // TODO(): Assuming image map is rgba -> which may possibly be rgb!
    img._memorySize = image.width * image.height * 4;
    
    pTex->Update(img);

    pTex->_Name = image.name;

    TextureCache::Cache(pTex);
    engineModel->textures.push_back(pTex);
  }
}


void LoadMaterials(tinygltf::Model* gltfModel, Model* engineModel)
{
  for (tinygltf::Material& mat : gltfModel->materials) {
    Material* engineMat = new Material();
    engineMat->Initialize();
    if (mat.values.find("baseColorTexture") != mat.values.end()) {
      engineMat->SetAlbedo(engineModel->textures[mat.values["baseColorTexture"].TextureIndex()]);
    }

    if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
      engineMat->SetNormal(engineModel->textures[mat.additionalValues["normalTexture"].TextureIndex()]);
    }


    MaterialCache::Cache(mat.name, engineMat);
    engineModel->materials.push_back(engineMat);
  }
}


ModelResult Load(const std::string path, Model* model)
{
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  std::string err;

  bool success = loader.LoadASCIIFromFile(&gltfModel, &err, path);
  if (!err.empty()) {
    Log() << err << "\n";
  }

  if (!success) {
    Log() << "Failed to parse glTF\n";
    return Model_Fail;
  }

  std::vector<Texture2D*> textures;
  LoadTextures(&gltfModel, model);
  LoadMaterials(&gltfModel, model);

  return Model_Success;
}

} // ModelLoader
} // Recluse