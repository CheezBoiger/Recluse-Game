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

    u8* pImgBuffer = nullptr;
    b8  bHeapAlloc = false;
    if (image.component == 3) {
      // From Sacha Willem's pbr gltf 2.0 work.
      // https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/base/VulkanglTFModel.hpp
      img._memorySize = image.width * image.height * 4;
      pImgBuffer = new u8[img._memorySize];
      u8* rgba = pImgBuffer;
      u8* rgb = image.image.data();
      for (size_t i = 0; i < image.width * image.height; ++i) {
        for (size_t j = 0; j < 3; ++j) {
          rgba[j] = rgb[j];
        }
        rgba += 4;
        rgb += 3;
      } 
      bHeapAlloc = true;
    } else {
      pImgBuffer = image.image.data();
      img._memorySize = image.image.size();
    }
  
    img._data = pImgBuffer;

    pTex->Update(img);
    
    if (bHeapAlloc) { delete pImgBuffer; }

    pTex->_Name = image.uri;

    TextureCache::Cache(pTex);
    engineModel->textures.push_back(pTex);
  }
}


void LoadMaterials(tinygltf::Model* gltfModel, Model* engineModel)
{
  for (tinygltf::Material& mat : gltfModel->materials) {
    Material* engineMat = new Material();
    engineMat->Initialize();
    engineMat->SetMetallicFactor(1.0f);
    engineMat->SetRoughnessFactor(1.0f);
    if (mat.values.find("baseColorTexture") != mat.values.end()) {
      engineMat->SetAlbedo(engineModel->textures[mat.values["baseColorTexture"].TextureIndex()]);
      engineMat->EnableAlbedo(true);
    }

    if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
      engineMat->SetNormal(engineModel->textures[mat.additionalValues["normalTexture"].TextureIndex()]);
      engineMat->EnableNormal(true);
    }

    if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {   
      engineMat->SetRoughnessMetallic(engineModel->textures[mat.values["metallicRoughnessTexture"].TextureIndex()]);
      engineMat->EnableRoughness(true);
      engineMat->EnableMetallic(true);
    }

    if (mat.values.find("roughnessFactor") != mat.values.end()) {
      engineMat->SetRoughnessFactor(static_cast<r32>(mat.values["roughnessFactor"].Factor()));
    } 

    if (mat.values.find("metallicFactor") != mat.values.end()) {
      engineMat->SetMetallicFactor(static_cast<r32>(mat.values["metallicFactor"].Factor()));
    }

    if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
      engineMat->SetEmissive(engineModel->textures[mat.additionalValues["emissiveTexture"].TextureIndex()]);
      engineMat->EnableEmissive(true);
    }


    MaterialCache::Cache(mat.name, engineMat);
    engineModel->materials.push_back(engineMat);
  }
}


void LoadNode(const tinygltf::Node& node, const tinygltf::Model& model, const Matrix4& parentMatrix, const r32 scale)
{
  Vector3 t;
  Matrix4 R;
  Vector3 s = Vector3(1.0f, 1.0f, 1.0f);
  if (node.translation.size() == 3) {
    const double* tnative = node.translation.data();
    t = Vector3(  static_cast<r32>(tnative[0]), 
                  static_cast<r32>(tnative[1]), 
                  static_cast<r32>(tnative[2]));
  }
  
  if (node.rotation.size() == 4) {
    const double* rq = node.rotation.data();
    Quaternion q = 
      Quaternion( static_cast<r32>(rq[0]),
                  static_cast<r32>(rq[1]),
                  static_cast<r32>(rq[2]),
                  static_cast<r32>(rq[3]));
    R = q.ToMatrix4();
  }

  if (node.scale.size() == 3) {
    const double* sv = node.scale.data();
    s = Vector3(  static_cast<r32>(sv[0]),
                  static_cast<r32>(sv[1]),
                  static_cast<r32>(sv[2]));
  }

  Matrix4 localMatrix = Matrix4::Identity();
  if (node.matrix.size() == 16) {
    localMatrix = Matrix4(node.matrix.data()).Transpose();
  } else {
    Matrix4 T = Matrix4::Translate(Matrix4::Identity(), t);
    Matrix4 S = Matrix4::Scale(Matrix4::Identity(), s);
    localMatrix = S * R * T;
  }
  if (!node.children.empty()) {
    for (size_t i = 0; i < node.children.size(); ++i) {
      LoadNode(model.nodes[node.children[i]], model, localMatrix, scale);
    }
  }

  if (node.mesh > -1) {
    const tinygltf::Mesh& mesh = model.meshes[node.mesh];
    // Mesh Should hold the fully buffer data. Primitives specify start and index count, that
    // defines some submesh in the full mesh object.
    //Mesh* pMesh = new Mesh();

    std::vector<StaticVertex> vertices;
    std::vector<u32>          indices;

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
      const tinygltf::Primitive& primitive = mesh.primitives[i];
      if (primitive.indices < 0) continue;
      R_ASSERT(primitive.attributes.find("POSITION") != primitive.attributes.end(), "No position values within mesh!");

      {
        const r32* bufferPositions = nullptr;
        const r32* bufferNormals = nullptr;
        const r32* bufferTexCoords = nullptr;

        const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView& bufViewPos = model.bufferViews[positionAccessor.bufferView];
        bufferPositions = 
          reinterpret_cast<const r32*>(&model.buffers[bufViewPos.buffer].data[positionAccessor.byteOffset + bufViewPos.byteOffset]);     

        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
          const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
          const tinygltf::BufferView& bufViewNorm = model.bufferViews[normalAccessor.bufferView];
          bufferNormals = 
            reinterpret_cast<const r32*>(&model.buffers[bufViewNorm.buffer].data[normalAccessor.byteOffset + bufViewNorm.byteOffset]);
        }

        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
          const tinygltf::Accessor& texcoordAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
          const tinygltf::BufferView& bufViewTexCoord0 = model.bufferViews[texcoordAccessor.bufferView];
          bufferTexCoords =
            reinterpret_cast<const r32*>(&model.buffers[bufViewTexCoord0.buffer].data[texcoordAccessor.byteOffset + bufViewTexCoord0.byteOffset]);
        }
     
        for (size_t value = 0; value < positionAccessor.count; ++value) {
          StaticVertex vertex;
          vertex.position = Vector4(Vector3(&bufferPositions[value * 3]), 1.0f);
          vertex.normal = Vector4(Vector3(&bufferNormals[value * 3]), 1.0f);
          vertex.texcoord0 = Vector2(&bufferTexCoords[value * 2]);
          vertex.texcoord1 = Vector2();
          vertices.push_back(vertex);
        }
      }

        // Indices.
      {
        const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& iBufView = model.bufferViews[indAccessor.bufferView];
        const tinygltf::Buffer& iBuf = model.buffers[iBufView.buffer];

        // TODO(): In progress. 
      }
    }
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

  LoadTextures(&gltfModel, model);
  LoadMaterials(&gltfModel, model);

  tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    tinygltf::Node& node = gltfModel.nodes[scene.nodes[i]];  
    LoadNode(node, gltfModel, Matrix4(), 1.0);
  }
  return Model_Success;
}

} // ModelLoader
} // Recluse