// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Scene/ModelLoader.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Image.hpp"
#include "Core/Exception.hpp"

#include "Game/Rendering/RendererResourcesCache.hpp"
#include "Rendering/TextureCache.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation/Clip.hpp"
#include "Game/Scene/AssetManager.hpp"

#include "Renderer/Vertex.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Renderer.hpp"

#include "tiny_gltf.hpp"
#include <queue>
#include <vector>
#include <stack>
#include <string>
#include <set>
#include <map>


#define SAMPLE_TRANSLATION_STRING   "translation"
#define SAMPLE_ROTATION_STRING      "rotation"
#define SAMPLE_SCALE_STRING         "scale"
#define SAMPLE_WEIGHTS_STRING       "weights"

namespace Recluse {
namespace ModelLoader {


void GeneratePrimitive(Primitive& handle, Material* mat, u32 firstIndex, u32 indexCount)
{
  Primitive prim;
  prim._pMat = mat;
  prim._firstIndex = firstIndex;
  prim._indexCount = indexCount;
  handle = std::move(prim);
}


static ModelResultBits LoadTextures(tinygltf::Model* gltfModel, Model* engineModel)
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

    pTex->_Name = engineModel->name + "_tex_";
    if (image.uri.empty()) {
      pTex->_Name += image.name;
    } else {
      pTex->_Name += image.uri;
    }

    TextureCache::Cache(pTex);
    engineModel->textures.push_back(pTex);
  }

  if ( gltfModel->textures.empty() ) {
    return Model_Textured;
  }
  return Model_None;
}


static ModelResultBits LoadMaterials(tinygltf::Model* gltfModel, Model* engineModel)
{
  u32 count = 0;
  for (tinygltf::Material& mat : gltfModel->materials) {
    Material* engineMat = new Material();
    engineMat->Initialize(&gRenderer());
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

    if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
      tinygltf::Parameter parameter = mat.additionalValues["alphaMode"];
      if (parameter.string_value == "BLEND") {
        engineMat->SetTransparent(true);
      }
      if (parameter.string_value == "MASK") {
        engineMat->SetTransparent(true);
      }
    }

    if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
      r32 factor = static_cast<r32>(mat.additionalValues["alphaCutoff"].Factor());
      engineMat->SetOpacity(factor);
    }

    std::string name = engineModel->name + "_mat_";
    // Some materials may not have a name, so will need to give them a unique name.
    if (mat.name.empty()) {
      name += std::to_string(count++);
    } else {
      name += mat.name;
    }
  
    MaterialCache::Cache(name, engineMat);
    engineModel->materials.push_back(engineMat);
  }

  if ( gltfModel->materials.empty() ) {
    return Model_Materials;
  }
  return Model_None;
}


static ModelResultBits LoadAnimations(tinygltf::Model* gltfModel, Model* engineModel)
{
  if (gltfModel->animations.empty()) return Model_None;
  
  for (size_t i = 0; i < gltfModel->animations.size(); ++i) {
    const tinygltf::Animation& animation = gltfModel->animations[i];
    AnimClip* clip = new AnimClip();
    clip->_name = animation.name;
    if (animation.name.empty()) {
      clip->_name = "Animation_" + std::to_string(engineModel->animations.size() + 1);
    }

    i32 prevTarget = -1;
    size_t jointIndex = -1;
    // channels follow the same pattern as its corresponding skeleton joint hierarchy.
    for (const tinygltf::AnimationChannel& channel : animation.channels) {
      i32 node = channel.target_node;
      if (node != prevTarget) { 
        prevTarget = node;
        ++jointIndex;
      }
      tinygltf::Node& tnode = gltfModel->nodes[node];

      const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];
      
      const tinygltf::Accessor& inputAccessor = gltfModel->accessors[sampler.input];
      const tinygltf::BufferView& inputBufView = gltfModel->bufferViews[inputAccessor.bufferView];
      const r32* inputValues = reinterpret_cast<const r32*>(&gltfModel->buffers[inputBufView.buffer].data[inputAccessor.byteOffset + inputBufView.byteOffset]);
      // Read input data.
      // TODO():
    
      const tinygltf::Accessor& outputAccessor = gltfModel->accessors[sampler.output];
      const tinygltf::BufferView& outputBufView = gltfModel->bufferViews[outputAccessor.bufferView];
      const r32* outputValues = reinterpret_cast<const r32*>(&gltfModel->buffers[outputBufView.buffer].data[outputAccessor.byteOffset + outputBufView.byteOffset]);
      // Read output data.
      // TODO():
      if (clip->_aAnimPoseSamples.size() < inputAccessor.count) {
        std::map<r32, AnimPose> poses;
        for (auto& pose : clip->_aAnimPoseSamples) {
          poses[pose._time] = std::move(pose);
        } 
        clip->_aAnimPoseSamples.resize(inputAccessor.count); 
        for (size_t inputId = 0; inputId < inputAccessor.count; ++inputId) {
            r32 kt = inputValues[inputId];
            if (poses.find(kt) == poses.end()) {
              clip->_aAnimPoseSamples[inputId]._time = kt;
            } else {
              clip->_aAnimPoseSamples[inputId] = poses[kt];
            }
        }
      }

      if (channel.target_path == SAMPLE_TRANSLATION_STRING) {
        for (size_t outputId = 0; outputId < outputAccessor.count; ++outputId) {
          AnimPose& pose = clip->_aAnimPoseSamples[outputId];
          if (jointIndex >= pose._aLocalPoses.size()) {
            pose._aGlobalPoses.resize(jointIndex + 1);
            pose._aLocalPoses.resize(jointIndex + 1);
          }
          pose._aLocalPoses[jointIndex]._trans = Vector3(outputValues[outputId * 3 + 0],
                                                         outputValues[outputId * 3 + 1],
                                                         outputValues[outputId * 3 + 2]);
          DEBUG_OP(pose._aLocalPoses[jointIndex]._id = node);
        }
      }
      if (channel.target_path == SAMPLE_ROTATION_STRING) {
        for (size_t outputId = 0; outputId < outputAccessor.count; ++outputId) {
          AnimPose& pose = clip->_aAnimPoseSamples[outputId];
          if (jointIndex >= pose._aLocalPoses.size()) {
            pose._aLocalPoses.resize(jointIndex + 1);
            pose._aGlobalPoses.resize(jointIndex + 1);
          }
          pose._aLocalPoses[jointIndex]._rot = Quaternion(outputValues[outputId * 4 + 0],
                                                          outputValues[outputId * 4 + 1],
                                                          outputValues[outputId * 4 + 2],
                                                          outputValues[outputId * 4 + 3]);

        }
      }
      if (channel.target_path == SAMPLE_SCALE_STRING) {
        for (size_t outputId = 0; outputId < outputAccessor.count; ++outputId) {
          AnimPose& pose = clip->_aAnimPoseSamples[outputId];
          if (jointIndex >= pose._aLocalPoses.size()) {
            pose._aLocalPoses.resize(jointIndex + 1);
            pose._aGlobalPoses.resize(jointIndex + 1);
          }
          pose._aLocalPoses[jointIndex]._scale = Vector3(outputValues[outputId * 3 + 0],
                                                         outputValues[outputId * 3 + 1],
                                                         outputValues[outputId * 3 + 2]);
          DEBUG_OP(pose._aLocalPoses[jointIndex]._id = node);
        }
      }
      if (channel.target_path == SAMPLE_WEIGHTS_STRING) {
        tinygltf::Mesh& tmesh = gltfModel->meshes[tnode.mesh];
        R_ASSERT(tnode.mesh != -1, "No target mesh.");
        size_t offset = tmesh.weights.size();
        size_t v = 0;
        for (size_t outputId = offset; outputId < outputAccessor.count; outputId += offset) {
          AnimPose& pose = clip->_aAnimPoseSamples[v];

          if (jointIndex >= pose._aLocalPoses.size()) {
            pose._aLocalPoses.resize(jointIndex + 1);
            pose._aGlobalPoses.resize(jointIndex + 1);
          }
          
          if (pose._morphs.size() < offset) {
            pose._morphs.resize(offset);
          }
        
          for (size_t n = 0; n < offset; ++n) {
            r32 weight = outputValues[outputId + n];
            // TODO(): Figure out how many morph targets in the animated mesh, in order to 
            // determine how to read this!
            pose._morphs[n] = weight;
          }
          ++v;
        }
      }
    }

    clip->_fDuration = clip->_aAnimPoseSamples[clip->_aAnimPoseSamples.size() - 1]._time;
    clip->_bLooping = true;
    clip->_fFps = 60.0f;
    // TODO(): Need to figure out how to target the skeleton for this clip.
    engineModel->animations.push_back(clip);
    AnimAssetManager::Cache(clip->_name, clip);
  }

  return Model_Animated;
}


static void FlipStaticTrianglesInArray(std::vector<StaticVertex>& vertices)
{
  for (size_t i = 0, count = vertices.size(); i < count - 2; i += 3)
    std::swap(vertices[i], vertices[i + 2]);
}


static Mesh* LoadMesh(const tinygltf::Node& node, const tinygltf::Model& model, Model* engineModel, Matrix4& localMatrix)
{
  Mesh* pMesh = nullptr;
  if (node.mesh > -1) {
    const tinygltf::Mesh& mesh = model.meshes[node.mesh];
    std::vector<Primitive> primitives;
    // Mesh Should hold the fully buffer data. Primitives specify start and index count, that
    // defines some submesh in the full mesh object.
    pMesh = new Mesh();
    
    std::vector<std::vector<MorphVertex> > morphVertices;
    std::vector<StaticVertex> vertices;
    std::vector<u32>          indices;
    Vector3                   min, max;

    if (!mesh.weights.empty()) {
      morphVertices.resize(mesh.weights.size());
    }

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
          vertex.normal = Vector4((Vector3(&bufferNormals[value * 3]) * Matrix3(localMatrix)).Normalize(), 0.0f);
          vertex.texcoord0 = bufferTexCoords ? Vector2(&bufferTexCoords[value * 2]) : Vector2(0.0f, 0.0f);
          vertex.texcoord0.y = vertex.texcoord0.y > 1.0f ? vertex.texcoord0.y - 1.0f : vertex.texcoord0.y;
          vertex.texcoord1 = Vector2();
          //vertex.position.y *= -1.0f;
          //vertex.normal.y *= -1.0f;
          vertices.push_back(vertex);
          min = Vector3::Min(min, p);
          max = Vector3::Max(max, p);
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

      // Check for each for morph target. For each target, we push to their corresponding maps.
      if (!primitive.targets.empty()) {
        for (size_t mi = 0; mi < primitive.targets.size(); ++mi) {
          std::map<std::string, int>& target = 
            const_cast<std::map<std::string, int>&>(primitive.targets[mi]);
          const r32*  morphPositions = nullptr;
          const r32* morphNormals = nullptr;
          const r32* morphTexCoords = nullptr;            
  
          const tinygltf::Accessor& morphPositionAccessor = model.accessors[target["POSITION"]];
          const tinygltf::BufferView& morphPositionView = model.bufferViews[morphPositionAccessor.bufferView];
          morphPositions = reinterpret_cast<const r32*>(&model.buffers[morphPositionView.buffer].data[morphPositionView.byteOffset + morphPositionAccessor.byteOffset]);
         
          if (target.find("NORMAL") != target.end()) {
            const tinygltf::Accessor& morphNormalAccessor = model.accessors[target["NORMAL"]];
            const tinygltf::BufferView& morphNormalView = model.bufferViews[morphNormalAccessor.bufferView];
            morphNormals = reinterpret_cast<const r32*>(&model.buffers[morphNormalView.buffer].data[morphNormalAccessor.byteOffset + morphNormalView.byteOffset]);
          }
        
          if (target.find("TEXCOORD_0") != target.end()) {
            const tinygltf::Accessor& morphTexCoordAccessor = model.accessors[target["TEXCOORD_0"]];
            const tinygltf::BufferView& morphTexCoordView = model.bufferViews[morphTexCoordAccessor.bufferView];
            morphTexCoords = reinterpret_cast<const r32*>(&model.buffers[morphTexCoordView.buffer].data[morphTexCoordAccessor.byteOffset + morphTexCoordView.byteOffset]);
          }

          for (size_t i = 0; i < morphPositionAccessor.count; ++i) {
            MorphVertex vertex;
            Vector3 p(&morphPositions[i * 3]);
            vertex.position = Vector4(p, 1.0f) * localMatrix;
            vertex.normal = Vector4(Vector3(&morphNormals[i * 3]) * Matrix3(localMatrix), 0.0f);
            vertex.texcoord0 = morphTexCoords ? Vector2(&morphTexCoords[i * 2]) : Vector2(0.0f, 0.0f);
            vertex.texcoord0.y = vertex.texcoord0.y > 1.0f ? vertex.texcoord0.y - 1.0f : vertex.texcoord0.y;
            morphVertices[mi].push_back(vertex);
          }
        }
      }


      Primitive primData;
      GeneratePrimitive(primData, engineModel->materials[primitive.material], indexStart, indexCount);
      primitives.push_back(primData);
    }

    pMesh->Initialize(&gRenderer(), vertices.size(), vertices.data(), Mesh::STATIC, indices.size(), indices.data());
    pMesh->SetMin(min);
    pMesh->SetMax(max);
    pMesh->UpdateAABB();
    std::string name = engineModel->name + "_mesh_" + mesh.name;
    MeshCache::Cache(name, pMesh);
    engineModel->meshes.push_back(pMesh);
    for (auto& prim : primitives) {
      pMesh->PushPrimitive(prim);
    }
    pMesh->SortPrimitives(Mesh::TRANSPARENCY_LAST);

    if (!morphVertices.empty()) {
      pMesh->AllocateMorphTargetBuffer(morphVertices.size());
      for ( size_t i = 0; i < morphVertices.size(); ++i ) {
        auto& verts = morphVertices[i];
        pMesh->InitializeMorphTarget(&gRenderer(), i, verts.size(), verts.data(), sizeof(MorphVertex)); 
      } 
    }
  }
  return pMesh;
}


static Mesh* LoadSkinnedMesh(const tinygltf::Node& node, const tinygltf::Model& model, Model* engineModel, Matrix4& localMatrix)
{
  Mesh* pMesh = nullptr;
  if (node.mesh > -1) {
    const tinygltf::Mesh& mesh = model.meshes[node.mesh];
    std::vector<Primitive> primitives;
    // Mesh Should hold the fully buffer data. Primitives specify start and index count, that
    // defines some submesh in the full mesh object.
    pMesh = new Mesh();

    std::vector<std::vector<MorphVertex> > morphVertices;
    std::vector<SkinnedVertex> vertices; 
    std::vector<u32>          indices;
    Vector3                   min, max;

    if (!mesh.weights.empty()) {
      morphVertices.resize(mesh.weights.size());
    }

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
        const std::vector<double>& dmin = positionAccessor.minValues;
        const std::vector<double>& dmax = positionAccessor.maxValues;

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
          min = Vector3::Min(min, p);
          max = Vector3::Max(max, p);
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

      // Check for each for morph target. For each target, we push to their corresponding maps.
      if (!primitive.targets.empty()) {
        for (size_t mi = 0; mi < primitive.targets.size(); ++mi) {
          std::map<std::string, int>& target =
            const_cast<std::map<std::string, int>&>(primitive.targets[mi]);
          const r32*  morphPositions = nullptr;
          const r32* morphNormals = nullptr;
          const r32* morphTexCoords = nullptr;

          const tinygltf::Accessor& morphPositionAccessor = model.accessors[target["POSITION"]];
          const tinygltf::BufferView& morphPositionView = model.bufferViews[morphPositionAccessor.bufferView];
          morphPositions = reinterpret_cast<const r32*>(&model.buffers[morphPositionView.buffer].data[morphPositionView.byteOffset + morphPositionAccessor.byteOffset]);

          if (target.find("NORMAL") != target.end()) {
            const tinygltf::Accessor& morphNormalAccessor = model.accessors[target["NORMAL"]];
            const tinygltf::BufferView& morphNormalView = model.bufferViews[morphNormalAccessor.bufferView];
            morphNormals = reinterpret_cast<const r32*>(&model.buffers[morphNormalView.buffer].data[morphNormalAccessor.byteOffset + morphNormalView.byteOffset]);
          }

          if (target.find("TEXCOORD_0") != target.end()) {
            const tinygltf::Accessor& morphTexCoordAccessor = model.accessors[target["TEXCOORD_0"]];
            const tinygltf::BufferView& morphTexCoordView = model.bufferViews[morphTexCoordAccessor.bufferView];
            morphTexCoords = reinterpret_cast<const r32*>(&model.buffers[morphTexCoordView.buffer].data[morphTexCoordAccessor.byteOffset + morphTexCoordView.byteOffset]);
          }

          for (size_t i = 0; i < morphPositionAccessor.count; ++i) {
            MorphVertex vertex;
            Vector3 p(&morphPositions[i * 3]);
            vertex.position = Vector4(p, 1.0f) * localMatrix;
            vertex.normal = Vector4(Vector3(&morphNormals[i * 3]) * Matrix3(localMatrix), 0.0f);
            vertex.texcoord0 = morphTexCoords ? Vector2(&morphTexCoords[i * 2]) : Vector2(0.0f, 0.0f);
            vertex.texcoord0.y = vertex.texcoord0.y > 1.0f ? vertex.texcoord0.y - 1.0f : vertex.texcoord0.y;
            morphVertices[mi].push_back(vertex);
          }
        }
      }

      Primitive primData;
      GeneratePrimitive(primData, engineModel->materials[primitive.material], indexStart, indexCount);

      // TODO():
      //    Still need to add start and index count.
      primitives.push_back(primData);
    }

    for (size_t i = 0; i < mesh.targets.size(); ++i) {
      auto target = mesh.targets[i];
    }

    pMesh->Initialize(&gRenderer(), vertices.size(), vertices.data(), Mesh::SKINNED, indices.size(), indices.data());
    pMesh->SetMin(min);
    pMesh->SetMax(max);
    pMesh->UpdateAABB();
    MeshCache::Cache(mesh.name, pMesh);
    engineModel->meshes.push_back(pMesh);
    for (auto& primData : primitives) {
      pMesh->PushPrimitive(primData);
    }
    pMesh->SortPrimitives(Mesh::TRANSPARENCY_LAST);

    if (!morphVertices.empty()) {
      pMesh->AllocateMorphTargetBuffer(morphVertices.size());
      for (size_t i = 0; i < morphVertices.size(); ++i) {
        auto& verts = morphVertices[i];
        pMesh->InitializeMorphTarget(&gRenderer(), i, verts.size(), verts.data(), sizeof(MorphVertex));
      }
    }
  }

  return pMesh;
}


struct NodeTransform {
  Vector3     _localTrans;
  Quaternion  _localRot;
  Vector3     _localScale;
  Matrix4     _globalMatrix;
};


static NodeTransform CalculateGlobalTransform(const tinygltf::Node& node, Matrix4 parentMatrix)
{
  NodeTransform transform;
  Vector3 t;
  Quaternion r;
  Vector3 s = Vector3(1.0f, 1.0f, 1.0f);
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

  transform._globalMatrix = localMatrix * parentMatrix;;
  transform._localRot = r;
  transform._localTrans = t;
  transform._localScale = s;
  return transform;
}

static skeleton_uuid_t LoadSkin(const tinygltf::Node& node, const tinygltf::Model& model, Model* engineModel, const Matrix4& parentMatrix)
{
  // TODO(): JointPoses are in the wrong order as invBinding matrices, need to sort them in the
  // order of joint array in GLTF file!!
  if (node.skin == -1) return Skeleton::kNoSkeletonId;

  Skeleton skeleton;
  tinygltf::Skin skin = model.skins[node.skin];
  b32 rootInJoints = false;
  for (size_t i = 0; i < skin.joints.size(); ++i) {
    if (skin.joints[i] == skin.skeleton) {
      rootInJoints = true; break;
    }
  }
  skeleton._joints.resize(skin.joints.size());
  skeleton._name = skin.name;
  skeleton._rootInJoints = rootInJoints;

  const tinygltf::Accessor& accessor = model.accessors[skin.inverseBindMatrices];
  const tinygltf::BufferView& bufView = model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buf = model.buffers[bufView.buffer];
  
  const r32* bindMatrices = reinterpret_cast<const r32*>(&buf.data[bufView.byteOffset + accessor.byteOffset]);  

  for (size_t i = 0; i < accessor.count; ++i) {
    Matrix4 invBindMat(&bindMatrices[i * 16]);
    Matrix4 bindMat = invBindMat.Inverse();
    bindMat = bindMat * parentMatrix;
    skeleton._joints[i]._InvBindPose = bindMat.Inverse();
  }

  struct NodeTag {
    u8                _parent;
    Matrix4           _parentTransform;
  };

  std::map<i32, NodeTag> nodeMap;
  if (skin.skeleton != -1) {
    const tinygltf::Node& root = model.nodes[skin.skeleton];
    NodeTransform rootTransform = CalculateGlobalTransform(root, 
      Matrix4::Scale(Matrix4(), Vector3(-1.0f, 1.0f, 1.0f)));
    skeleton._rootInvTransform = rootTransform._globalMatrix.Inverse();
    NodeTag tag{ 0xff, Matrix4() };
    nodeMap[skin.skeleton] = tag;
    for (size_t i = 0; i < root.children.size(); ++i) {
      NodeTag tag = { (rootInJoints ? static_cast<u8>(0) : static_cast<u8>(0xff)), 
        rootTransform._globalMatrix };
      nodeMap[root.children[i]] = tag;
    }
  }

  for (size_t i = 0; i < skin.joints.size(); ++i) {
    size_t idx = i;
    Joint& joint = skeleton._joints[idx];
    i32 skinJointIdx = skin.joints[i];
    const tinygltf::Node& node = model.nodes[skinJointIdx];
    NodeTransform localTransform;

    auto it = nodeMap.find(skinJointIdx);
    if (it != nodeMap.end()) {
      NodeTag& tag = it->second;
      localTransform = CalculateGlobalTransform(node, tag._parentTransform);    
      joint._iParent = tag._parent;
    }

    DEBUG_OP(joint._id = static_cast<u8>(skinJointIdx));
    for (size_t child = 0; child < node.children.size(); ++child) {
      NodeTag tag = { static_cast<u8>(idx), localTransform._globalMatrix };
      nodeMap[node.children[child]] = tag;
    }
  }
  
  Skeleton::PushSkeleton(skeleton);
  
  engineModel->skeletons.push_back(Skeleton::GetSkeleton(skeleton._uuid));
  return skeleton._uuid;
}


static void LoadNode(const tinygltf::Node& node, const tinygltf::Model& model, Model* engineModel, const Matrix4& parentMatrix, const r32 scale)
{
  NodeTransform transform = CalculateGlobalTransform(node, parentMatrix);
  if (!node.children.empty()) {
    for (size_t i = 0; i < node.children.size(); ++i) {
      LoadNode(model.nodes[node.children[i]], model, engineModel, transform._globalMatrix, scale);
    }
  }

  if (node.skin != -1) {
    skeleton_uuid_t skeleId = LoadSkin(node, model, engineModel, transform._globalMatrix);
    Mesh* pMesh = LoadSkinnedMesh(node, model, engineModel, transform._globalMatrix);
    pMesh->SetSkeletonReference(skeleId);
  }
  else {
    LoadMesh(node, model, engineModel, transform._globalMatrix);
  }
}


void GetFilenameAndType(const std::string& path, std::string& filenameOut, u32& typeOut)
{
  size_t cutoff = path.find_last_of('/');
  if (cutoff != std::string::npos) {
    size_t removeExtId = path.find_last_of('.');
    if (removeExtId != std::string::npos) {
      filenameOut = std::move(path.substr(cutoff + 1, removeExtId - (cutoff + 1)));
      std::string ext = path.substr(removeExtId, path.size());
      if (ext.compare(".glb") == 0) {
        typeOut = 1;
      }
      else {
        typeOut = 0;
      }
    }
  }
}


ModelResultBits Load(const std::string path)
{
  ModelResultBits result = 0;
  Model*           model = nullptr;
  static u64 copy = 0;
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string modelName = "Unknown" + std::to_string(copy++);
  u32 type = 0;
  GetFilenameAndType(path, modelName, type);

  bool success = type == 1 ? loader.LoadBinaryFromFile(&gltfModel, &err, path) 
    : loader.LoadASCIIFromFile(&gltfModel, &err, path);

  if (!err.empty()) {
    Log() << err << "\n";
  }

  if (!success) {
    Log() << "Failed to parse glTF\n";
    return Model_Fail;
  }

  // Successful loading from tinygltf.
  model = new Model();
  model->name = std::move(modelName);

  result |= LoadTextures(&gltfModel, model);
  result |= LoadMaterials(&gltfModel, model);

  tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    tinygltf::Node& node = gltfModel.nodes[scene.nodes[i]];
    Matrix4 mat = Matrix4::Scale(Matrix4::Identity(), Vector3(-1.0f, 1.0f, 1.0f));
    LoadNode(node, gltfModel, model, mat, 1.0);
  }

  result |= LoadAnimations(&gltfModel, model);

  ModelCache::Cache(model->name, model);
  result |= Model_Cached;

  result |= Model_Success;
  return result;
}
} // ModelLoader
} // Recluse