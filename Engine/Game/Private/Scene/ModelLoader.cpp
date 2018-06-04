// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Scene/ModelLoader.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Image.hpp"
#include "Core/Exception.hpp"

#include "Game/Rendering/RendererResourcesCache.hpp"
#include "Rendering/TextureCache.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation/Clip.hpp"

#include "Renderer/Vertex.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Renderer.hpp"

#include "tiny_gltf.hpp"
#include <queue>

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
        rgba[3] = 0xff; // For opaque
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

    if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
      engineMat->SetAo(engineModel->textures[mat.additionalValues["occlusionTexture"].TextureIndex()]);
      engineMat->EnableAo(true);
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

    if (mat.values.find("baseColorFactor") != mat.values.end()) {
      tinygltf::ColorValue& value = mat.values["baseColorFactor"].ColorFactor();
      engineMat->SetBaseColor(Vector4(static_cast<r32>(value[0]), 
                                      static_cast<r32>(value[1]), 
                                      static_cast<r32>(value[2]), 
                                      static_cast<r32>(value[3])));
    }


    MaterialCache::Cache(mat.name, engineMat);
    engineModel->materials.push_back(engineMat);
  }
}


void LoadAnimations(tinygltf::Model* gltfModel, Model* engineModel)
{
  if (gltfModel->animations.empty()) return;
  for (size_t i = 0; i < gltfModel->animations.size(); ++i) {
    const tinygltf::Animation& animation = gltfModel->animations[i];
    //AnimClip* clip = new AnimClip();
    for (const tinygltf::AnimationChannel& channel : animation.channels) {
      const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];
      
      const tinygltf::Accessor& inputAccessor = gltfModel->accessors[sampler.input];
      const tinygltf::BufferView& inputBufView = gltfModel->bufferViews[inputAccessor.bufferView];
      const r32* inputValues = reinterpret_cast<const r32*>(&gltfModel->buffers[inputBufView.buffer].data[inputAccessor.byteOffset + inputBufView.byteOffset]);
      // Read input data.
    
      const tinygltf::Accessor& outputAccessor = gltfModel->accessors[sampler.output];
      const tinygltf::BufferView& outputBufView = gltfModel->bufferViews[outputAccessor.bufferView];
      const r32* outputValues = reinterpret_cast<const r32*>(&gltfModel->buffers[outputBufView.buffer].data[outputAccessor.byteOffset + outputBufView.byteOffset]);
      // Read output data.

    }
  }
}


void FlipStaticTrianglesInArray(std::vector<StaticVertex>& vertices)
{
  for (size_t i = 0, count = vertices.size(); i < count - 2; i += 3)
    std::swap(vertices[i], vertices[i + 2]);
}


void LoadMesh(const tinygltf::Node& node, const tinygltf::Model& model, Model* engineModel, Matrix4& localMatrix)
{
  if (node.mesh > -1) {
    const tinygltf::Mesh& mesh = model.meshes[node.mesh];
    // Mesh Should hold the fully buffer data. Primitives specify start and index count, that
    // defines some submesh in the full mesh object.
    Mesh* pMesh = new Mesh();

    std::vector<StaticVertex> vertices;
    std::vector<u32>          indices;

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
      const tinygltf::Primitive& primitive = mesh.primitives[i];
      u32   vertexStart = static_cast<u32>(vertices.size());
      u32   indexStart = static_cast<u32>(indices.size());
      u32   indexCount = 0;
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
          Vector3 p(&bufferPositions[value * 3]);
          vertex.position = Vector4(p, 1.0f) * localMatrix;
          vertex.position.w = 1.0f;
          vertex.normal = Vector4(Vector3(&bufferNormals[value * 3]) * Matrix3(localMatrix), 0.0f);
          vertex.texcoord0 = bufferTexCoords ? Vector2(&bufferTexCoords[value * 2]) : Vector2(0.0f, 0.0f);
          vertex.texcoord0.y = vertex.texcoord0.y > 1.0f ? vertex.texcoord0.y - 1.0f : vertex.texcoord0.y;
          vertex.texcoord1 = Vector2();
          //vertex.position.y *= -1.0f;
          //vertex.normal.y *= -1.0f;
          vertices.push_back(vertex);
        }
      }

      // Indices.
      {
        const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& iBufView = model.bufferViews[indAccessor.bufferView];
        const tinygltf::Buffer& iBuf = model.buffers[iBufView.buffer];
        indexCount = static_cast<u32>(indAccessor.count);

        // TODO(): In progress. 
        switch (indAccessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
          const u32* buf = (const u32*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(buf[index] + vertexStart);
          }
        } break;
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
          const u16* buf = (const u16*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(((u32)buf[index]) + vertexStart);
          }
        } break;
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
          const u8* buf = (const u8*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(((u32)buf[index]) + vertexStart);
          }
        } break;
        };
      }

      Primitive prim;
      prim._pMesh = pMesh->Native();
      prim._pMat = primitive.material != -1 ? engineModel->materials[primitive.material]->Native() : nullptr;
      prim._firstIndex = indexStart;
      prim._indexCount = indexCount;
      PrimitiveHandle primData;
      primData._primitive = prim;
      primData._pMaterial = engineModel->materials[primitive.material];
      primData._pMesh = pMesh;
      // TODO():
      //    Still need to add start and index count.
      engineModel->primitives.push_back(primData);
    }

    pMesh->Initialize(vertices.size(), vertices.data(), MeshData::STATIC, indices.size(), indices.data());
    MeshCache::Cache(mesh.name, pMesh);
    engineModel->meshes.push_back(pMesh);
  }
}


void LoadSkinnedMesh(const tinygltf::Node& node, const tinygltf::Model& model, Model* engineModel, Matrix4& localMatrix)
{
  if (node.mesh > -1) {
    const tinygltf::Mesh& mesh = model.meshes[node.mesh];
    // Mesh Should hold the fully buffer data. Primitives specify start and index count, that
    // defines some submesh in the full mesh object.
    Mesh* pMesh = new Mesh();

    std::vector<SkinnedVertex> vertices;
    std::vector<u32>          indices;

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
      const tinygltf::Primitive& primitive = mesh.primitives[i];
      u32   vertexStart = static_cast<u32>(vertices.size());
      u32   indexStart = static_cast<u32>(indices.size());
      u32   indexCount = 0;
      if (primitive.indices < 0) continue;
      R_ASSERT(primitive.attributes.find("POSITION") != primitive.attributes.end(), "No position values within mesh!");

      {
        const r32* bufferPositions = nullptr;
        const r32* bufferNormals = nullptr;
        const r32* bufferTexCoords = nullptr;
        const r32* bufferWeights = nullptr; 
        const u8* bufferJoints = nullptr;
        i32 jointType = -1;

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

        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
          const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
          const tinygltf::BufferView& bufferViewJoints = model.bufferViews[jointAccessor.bufferView];
          bufferJoints = 
            reinterpret_cast<const u8*>(&model.buffers[bufferViewJoints.buffer].data[jointAccessor.byteOffset + bufferViewJoints.byteOffset]);
          jointType = jointAccessor.componentType;
        }

        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
          const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
          const tinygltf::BufferView& bufferViewWeight = model.bufferViews[weightAccessor.bufferView];
          bufferWeights =
            reinterpret_cast<const r32*>(&model.buffers[bufferViewWeight.buffer].data[weightAccessor.byteOffset + bufferViewWeight.byteOffset]);
        }

        for (size_t value = 0; value < positionAccessor.count; ++value) {
          SkinnedVertex vertex;
          null_bones(vertex);
          Vector3 p(&bufferPositions[value * 3]);
          vertex.position = Vector4(p, 1.0f) * localMatrix;
          vertex.position.w = 1.0f;
          vertex.normal = Vector4(Vector3(&bufferNormals[value * 3]) * Matrix3(localMatrix), 0.0f);
          vertex.texcoord0 = bufferTexCoords ? Vector2(&bufferTexCoords[value * 2]) : Vector2(0.0f, 0.0f);
          vertex.texcoord0.y = vertex.texcoord0.y > 1.0f ? vertex.texcoord0.y - 1.0f : vertex.texcoord0.y;
          vertex.texcoord1 = Vector2();
          if (bufferWeights && bufferJoints) {
            vertex.boneWeights = Vector4(&bufferWeights[value * 4]);
            switch (jointType) {
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
              {
                vertex.boneIds[0] = (i32)((u16*)bufferJoints)[value * 4 + 0];
                vertex.boneIds[1] = (i32)((u16*)bufferJoints)[value * 4 + 1];
                vertex.boneIds[2] = (i32)((u16*)bufferJoints)[value * 4 + 2];
                vertex.boneIds[3] = (i32)((u16*)bufferJoints)[value * 4 + 3];
              } break;
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
              default: 
              {
                vertex.boneIds[0] = (i32)bufferJoints[value * 4 + 0];
                vertex.boneIds[1] = (i32)bufferJoints[value * 4 + 1];
                vertex.boneIds[2] = (i32)bufferJoints[value * 4 + 2];
                vertex.boneIds[3] = (i32)bufferJoints[value * 4 + 3];
              }break;
            }
          }
          //vertex.position.y *= -1.0f;
          //vertex.normal.y *= -1.0f;
          vertices.push_back(vertex);
        }
      }

      // Indices.
      {
        const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& iBufView = model.bufferViews[indAccessor.bufferView];
        const tinygltf::Buffer& iBuf = model.buffers[iBufView.buffer];
        indexCount = static_cast<u32>(indAccessor.count);

        // TODO(): In progress. 
        switch (indAccessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
          const u32* buf = (const u32*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(buf[index] + vertexStart);
          }
        } break;
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
          const u16* buf = (const u16*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(((u32)buf[index]) + vertexStart);
          }
        } break;
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
          const u8* buf = (const u8*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(((u32)buf[index]) + vertexStart);
          }
        } break;
        };
      }

      Primitive prim;
      prim._pMesh = pMesh->Native();
      prim._pMat = primitive.material != -1 ? engineModel->materials[primitive.material]->Native() : nullptr;
      prim._firstIndex = indexStart;
      prim._indexCount = indexCount;
      PrimitiveHandle primData;
      primData._pMaterial = engineModel->materials[primitive.material];
      primData._pMesh = pMesh;
      primData._primitive = prim;

      // TODO():
      //    Still need to add start and index count.
      engineModel->primitives.push_back(std::move(primData));
    }

    pMesh->Initialize(vertices.size(), vertices.data(), MeshData::SKINNED, indices.size(), indices.data());
    MeshCache::Cache(mesh.name, pMesh);
    engineModel->meshes.push_back(pMesh);
  }
}


void LoadSkin(const tinygltf::Node& node, const tinygltf::Model& model, AnimModel* engineModel, const Matrix4& parentMatrix)
{
  if (node.skin == -1) return;

  Skeleton skeleton;
  tinygltf::Skin skin = model.skins[node.skin];
  skeleton._joints.resize(skin.joints.size());
  skeleton._name = skin.name;

  const tinygltf::Accessor& accessor = model.accessors[skin.inverseBindMatrices];
  const tinygltf::BufferView& bufView = model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buf = model.buffers[bufView.buffer];
  
  const r32* bindMatrices = reinterpret_cast<const r32*>(&buf.data[bufView.byteOffset + accessor.byteOffset]);  

  for (size_t i = 0; i < skeleton._joints.size(); ++i) {
    Matrix4 invBindMat(&bindMatrices[i * 16]);
    skeleton._joints[i]._InvBindPose = invBindMat;
  }

  // Traverse joint information.
  struct NodeTag {
    const tinygltf::Node* _pNode;
    u8                    _parent;
  };

  std::queue<NodeTag> nodes;
  // extract skeleton root children joints.
  for (size_t i = 0; i < model.nodes[skin.skeleton].children.size(); ++i) {
    const tinygltf::Node& node = model.nodes[model.nodes[skin.skeleton].children[i]]; 
    nodes.push({ &node, 0xffu });
  }

  // TODO(): Figure out what we need to do with global transform bind pose.

  // now traverse skeleton joints.
  u8 idx = 0;
  while (!nodes.empty()) {
    NodeTag& tag = nodes.front();
    const tinygltf::Node* node = tag._pNode;
    for (size_t i = 0; i < node->children.size(); ++i) {
      nodes.push({ &model.nodes[node->children[i]], idx });
    }

    Joint& joint = skeleton._joints[idx];
    joint._name = node->name;
    joint._iParent = tag._parent;
    nodes.pop();
    ++idx;
  } 

  Skeleton::PushSkeleton(skeleton);
  engineModel->skeletons.push_back(&Skeleton::GetSkeleton(skeleton._uuid));
}


void LoadSkinnedNode(const tinygltf::Node& node, const tinygltf::Model& model, AnimModel* engineModel, const Matrix4& parentMatrix, const r32 scale)
{
  Vector3 t;
  Quaternion r;
  Vector3 s = Vector3(1.0f, 1.0f, 1.0f); // Reversing z coords, since we are in a left hand coordinate system.
  if (node.translation.size() == 3) {
    const double* tnative = node.translation.data();
    t = Vector3(static_cast<r32>(tnative[0]),
      static_cast<r32>(tnative[1]),
      static_cast<r32>(tnative[2]));
  }

  if (node.rotation.size() == 4) {
    const double* rq = node.rotation.data();
    r = Quaternion(static_cast<r32>(rq[0]),
      static_cast<r32>(rq[1]),
      static_cast<r32>(rq[2]),
      static_cast<r32>(rq[3]));
    //r = r.Inverse();
  }

  if (node.scale.size() == 3) {
    const double* sv = node.scale.data();
    s = Vector3(static_cast<r32>(sv[0]),
      static_cast<r32>(sv[1]),
      static_cast<r32>(sv[2]));
  }

  Matrix4 localMatrix = Matrix4::Identity();
  if (node.matrix.size() == 16) {
    localMatrix = Matrix4(node.matrix.data());
  } else {
    Matrix4 T = Matrix4::Translate(Matrix4::Identity(), t);
    Matrix4 R = r.ToMatrix4();
    Matrix4 S = Matrix4::Scale(Matrix4::Identity(), s);
    localMatrix = S * R * T;
  }
  localMatrix = localMatrix * parentMatrix;

  // if skin, this must be the root.
  if (!node.children.empty()) {
    for (size_t i = 0; i < node.children.size(); ++i) {
      LoadSkinnedNode(model.nodes[node.children[i]], model, engineModel, localMatrix, scale);
    }
  }

  LoadSkin(node, model, engineModel, localMatrix);
  LoadSkinnedMesh(node, model, engineModel, localMatrix);
  // TODO(): Load animations from gltf.
}


void LoadNode(const tinygltf::Node& node, const tinygltf::Model& model, Model* engineModel, const Matrix4& parentMatrix, const r32 scale)
{
  Vector3 t;
  Quaternion r;
  Vector3 s = Vector3(1.0f, 1.0f, 1.0f); // Reversing z coords, since we are in a left hand coordinate system.
  if (node.translation.size() == 3) {
    const double* tnative = node.translation.data();
    t = Vector3(static_cast<r32>(tnative[0]),
      static_cast<r32>(tnative[1]),
      static_cast<r32>(tnative[2]));
  }

  if (node.rotation.size() == 4) {
    const double* rq = node.rotation.data();
    r = Quaternion(static_cast<r32>(rq[0]),
      static_cast<r32>(rq[1]),
      static_cast<r32>(rq[2]),
      static_cast<r32>(rq[3]));
    //r = r.Inverse();
  }

  if (node.scale.size() == 3) {
    const double* sv = node.scale.data();
    s = Vector3(static_cast<r32>(sv[0]),
      static_cast<r32>(sv[1]),
      static_cast<r32>(sv[2]));
  }

  Matrix4 localMatrix = Matrix4::Identity();
  if (node.matrix.size() == 16) {
    localMatrix = Matrix4(node.matrix.data()).Transpose();
  }
  else {
    Matrix4 T = Matrix4::Translate(Matrix4::Identity(), t);
    Matrix4 R = r.ToMatrix4();
    Matrix4 S = Matrix4::Scale(Matrix4::Identity(), s);
    localMatrix = S * R * T;
  }
  localMatrix = localMatrix * parentMatrix;
  if (!node.children.empty()) {
    for (size_t i = 0; i < node.children.size(); ++i) {
      LoadNode(model.nodes[node.children[i]], model, engineModel, localMatrix, scale);
    }
  }

  LoadMesh(node, model, engineModel, localMatrix);
}


ModelResult Load(const std::string path)
{
  Model*           model = nullptr;
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

  // Successful loading from tinygltf.
  model = new Model();

  LoadTextures(&gltfModel, model);
  LoadMaterials(&gltfModel, model);

  tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    tinygltf::Node& node = gltfModel.nodes[scene.nodes[i]];
    Matrix4 mat = Matrix4::Scale(Matrix4::Identity(), Vector3(-1.0f, 1.0f, 1.0f));
    LoadNode(node, gltfModel, model, mat, 1.0);
  }

  static u64 copy = 0;
  model->name = "Unknown" + std::to_string(copy++);
  size_t cutoff = path.find_last_of('/');
  if (cutoff != std::string::npos) {
    size_t removeExtId = path.find_last_of('.');
    if (removeExtId != std::string::npos) {
      std::string fileName = path.substr(cutoff + 1, removeExtId - (cutoff + 1));
      model->name = fileName;
    }
  }
  ModelCache::Cache(model->name, model);
  return Model_Success;
}


ModelResult LoadAnimatedModel(const std::string path)
{
  AnimModel*      model = nullptr;
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

  // Successful loading from tinygltf.
  model = new AnimModel();

  LoadTextures(&gltfModel, model);
  LoadMaterials(&gltfModel, model);

  tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    tinygltf::Node& node = gltfModel.nodes[scene.nodes[i]];
    Matrix4 mat = Matrix4::Scale(Matrix4::Identity(), Vector3(-1.0f, 1.0f, 1.0f));
    LoadSkinnedNode(node, gltfModel, model, mat, 1.0);
  }

  LoadAnimations(&gltfModel, model);

  static u64 copy = 0;
  model->name = "Unknown" + std::to_string(copy++);
  size_t cutoff = path.find_last_of('/');
  if (cutoff != std::string::npos) {
    size_t removeExtId = path.find_last_of('.');
    if (removeExtId != std::string::npos) {
      std::string fileName = path.substr(cutoff + 1, removeExtId - (cutoff + 1));
      model->name = fileName;
    }
  }
  ModelCache::Cache(model->name, model);
  return Model_Success;
}
} // ModelLoader
} // Recluse