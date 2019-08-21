// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "ModelLoaderGLTF.hpp"
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
#include <unordered_map>


#define SAMPLE_TRANSLATION_STRING   "translation"
#define SAMPLE_ROTATION_STRING      "rotation"
#define SAMPLE_SCALE_STRING         "scale"
#define SAMPLE_WEIGHTS_STRING       "weights"

namespace std {


void hash_combine(size_t& seed, size_t hash) 
{
  hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
  seed ^= hash;
}

template<> struct hash<Recluse::Vector2>
{
  size_t operator()(Recluse::Vector2 const& vec) const
  {
    size_t seed = 0;
    std::hash<Recluse::R32> hasher;
    hash_combine(seed, hasher(vec.x));
    hash_combine(seed, hasher(vec.y));
    return seed;
  }
};


template<> struct hash<Recluse::Vector3> 
{
  size_t operator()(Recluse::Vector3 const& vec) const 
  {
    size_t seed = 0;
    std::hash<Recluse::R32> hasher;
    hash_combine(seed, hasher(vec.x));
    hash_combine(seed, hasher(vec.y));
    hash_combine(seed, hasher(vec.z));
    return seed;
  }
};


template<> struct hash<Recluse::Vector4> 
{
  size_t operator()(Recluse::Vector4 const& vec) const 
  {
    size_t seed = 0;
    std::hash<Recluse::R32> hasher;
    hash_combine(seed, hasher(vec.x));
    hash_combine(seed, hasher(vec.y));
    hash_combine(seed, hasher(vec.z));
    hash_combine(seed, hasher(vec.w));
    return seed;
  }
};

template<> struct hash<Recluse::StaticVertex> 
{
  size_t operator()(Recluse::StaticVertex const& vertex) const 
  {
    return (( hash<Recluse::Vector4>()(vertex.position) ^
            ( hash<Recluse::Vector4>()(vertex.normal) << 1)) >> 1) ^
            ( hash<Recluse::Vector2>()(vertex.texcoord0) << 1) ^
            ( hash<Recluse::Vector2>()(vertex.texcoord1) << 1);
  }
};


}

namespace Recluse {

namespace ModelLoader {
namespace GLTF {

void GeneratePrimitive(Primitive& handle, Material* mat, U32 firstIndex, U32 indexCount)
{
  handle._pMat = mat;
  handle._firstIndex = firstIndex;
  handle._indexCount = indexCount;
  handle._localConfigs = 0;
}


static ModelResultBits LoadTextures(tinygltf::Model* gltfModel, Model* engineModel)
{
  for (tinygltf::Image& image : gltfModel->images) {
    Texture2D* pTex = gRenderer().createTexture2D();
    pTex->initialize(RFORMAT_R8G8B8A8_UNORM, static_cast<U32>(image.width),
                  static_cast<U32>(image.height), true);
    Image img;

    U8* pImgBuffer = nullptr;
    B8  bHeapAlloc = false;
    if (image.component == 3) {
      // From Sacha Willem's pbr gltf 2.0 work.
      // https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/base/VulkanglTFModel.hpp
      img._memorySize = image.width * image.height * 4;
      pImgBuffer = new U8[img._memorySize];
      U8* rgba = pImgBuffer;
      U8* rgb = image.image.data();
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

    pTex->update(img);
    
    if (bHeapAlloc) { delete pImgBuffer; }

    pTex->_Name = engineModel->name + "_tex_";
    if (image.uri.empty()) {
      pTex->_Name += image.name;
    } else {
      pTex->_Name += image.uri;
    }

    TextureCache::cache(pTex);
    engineModel->textures.push_back(pTex);
  }

  if ( gltfModel->textures.empty() ) {
    return Model_Textured;
  }
  return Model_None;
}


static SamplerAddressMode GetSamplerAddressMode(I32 wrap)
{
    switch (wrap) {
      case TINYGLTF_TEXTURE_WRAP_REPEAT: return SAMPLER_ADDRESS_REPEAT;
      case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return SAMPLER_ADDRESS_CLAMP_TO_EDGE;
      case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return SAMPLER_ADDRESS_MIRRORED_REPEAT;
      default: return SAMPLER_ADDRESS_REPEAT;
    }
}


static void InitSamplerFilterMode(SamplerInfo& info, I32 minFilter, I32 magFilter)
{
  switch (minFilter) {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    {
      info._minFilter = SAMPLER_FILTER_NEAREST;
    } break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
    default:
    {
      info._minFilter = SAMPLER_FILTER_LINEAR;
    }
  }

  switch (magFilter) {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    {
      info._maxFilter = SAMPLER_FILTER_NEAREST;
    } break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
    default:
    {
      info._maxFilter = SAMPLER_FILTER_LINEAR;
    }
  }

  if (minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR
    || minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR
    || minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR) {
    info._mipmapMode = SAMPLER_MIPMAP_MODE_LINEAR;
  }

  if (minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST
    || minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST
    || minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) {
    info._mipmapMode = SAMPLER_MIPMAP_MODE_NEAREST;
  }
}


static ModelResultBits LoadSamplers(tinygltf::Model* gltfModel, Model* engineModel)
{
  for (auto& sampler : gltfModel->samplers) {
    SamplerInfo samplerInfo = { };
    samplerInfo._addrU = GetSamplerAddressMode(sampler.wrapS);
    samplerInfo._addrV = GetSamplerAddressMode(sampler.wrapT);
    samplerInfo._addrW = GetSamplerAddressMode(sampler.wrapR);
    InitSamplerFilterMode(samplerInfo, sampler.minFilter, sampler.magFilter);
    samplerInfo._borderColor = SAMPLER_BORDER_COLOR_OPAQUE_WHITE;
    samplerInfo._enableAnisotropy = false;
    samplerInfo._maxAniso = 16.0f;
    samplerInfo._maxLod = 32.0f;
    samplerInfo._minLod = 0.0f;
    samplerInfo._unnnormalizedCoordinates = false;
    TextureSampler* pSampler = gRenderer().createTextureSampler(samplerInfo);
    SamplerCache::cache(pSampler);
    engineModel->samplers.push_back(pSampler);
  }
  return Model_None;
}


static ModelResultBits LoadMaterials(tinygltf::Model* gltfModel, Model* engineModel)
{
  U32 count = 0;
  for (tinygltf::Material& mat : gltfModel->materials) {
    Material* engineMat = new Material();
    engineMat->initialize(&gRenderer());
    engineMat->setMetallicFactor(1.0f);
    engineMat->setRoughnessFactor(1.0f);
    if (mat.values.find("baseColorTexture") != mat.values.end()) { 
      tinygltf::Texture& texture = gltfModel->textures[mat.values["baseColorTexture"].TextureIndex()]; 
      engineMat->setAlbedo(engineModel->textures[mat.values["baseColorTexture"].TextureIndex()]);
      if (texture.sampler != -1) engineMat->setAlbedoSampler(engineModel->samplers[texture.sampler]);
      engineMat->enableAlbedo(true);
    }

    if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
      engineMat->setNormal(engineModel->textures[mat.additionalValues["normalTexture"].TextureIndex()]);
      tinygltf::Texture& texture = gltfModel->textures[mat.additionalValues["normalTexture"].TextureIndex()];
      if (texture.sampler != -1) engineMat->setNormalSampler(engineModel->samplers[texture.sampler]);
      engineMat->enableNormal(true);
    }

    if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {   
      engineMat->setRoughnessMetallic(engineModel->textures[mat.values["metallicRoughnessTexture"].TextureIndex()]);
      tinygltf::Texture& texture = gltfModel->textures[mat.values["metallicRoughnessTexture"].TextureIndex()]; 
      if (texture.sampler != -1) engineMat->setRoughMetalSampler(engineModel->samplers[texture.sampler]);
      engineMat->enableRoughness(true);
      engineMat->enableMetallic(true);
    }

    if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
      engineMat->setAo(engineModel->textures[mat.additionalValues["occlusionTexture"].TextureIndex()]);
      tinygltf::Texture& texture = gltfModel->textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
      if (texture.sampler != -1) engineMat->setAoSampler(engineModel->samplers[texture.sampler]);
      engineMat->enableAo(true);
    }

    if (mat.values.find("roughnessFactor") != mat.values.end()) {
      engineMat->setRoughnessFactor(static_cast<R32>(mat.values["roughnessFactor"].Factor()));
    } 

    if (mat.values.find("metallicFactor") != mat.values.end()) {
      engineMat->setMetallicFactor(static_cast<R32>(mat.values["metallicFactor"].Factor()));
    }

    if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
      engineMat->setEmissive(engineModel->textures[mat.additionalValues["emissiveTexture"].TextureIndex()]);
      tinygltf::Texture& texture = gltfModel->textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
      if (texture.sampler != -1) engineMat->setEmissiveSampler(engineModel->samplers[texture.sampler]);
      engineMat->enableEmissive(true);
    }

    if (mat.values.find("baseColorFactor") != mat.values.end()) {
      tinygltf::ColorValue& value = mat.values["baseColorFactor"].ColorFactor();
      engineMat->setBaseColor(Vector4(static_cast<R32>(value[0]), 
                                      static_cast<R32>(value[1]), 
                                      static_cast<R32>(value[2]), 
                                      static_cast<R32>(value[3])));
    }

    if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
      tinygltf::Parameter parameter = mat.additionalValues["alphaMode"];
      if (parameter.string_value == "BLEND") {
        engineMat->setTransparent(true);
      }
      if (parameter.string_value == "MASK") {
        engineMat->setTransparent(true);
      }
    }

    if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
      R32 factor = static_cast<R32>(mat.additionalValues["alphaCutoff"].Factor());
      engineMat->setOpacity(factor);
    }

    std::string name = engineModel->name + "_mat_";
    // Some materials may not have a name, so will need to give them a unique name.
    if (mat.name.empty()) {
      name += std::to_string(count++);
    } else {
      name += mat.name;
    }
  
    MaterialCache::cache(name, engineMat);
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

    I32 prevTarget = -1;
    size_t jointIndex = -1;
    // channels follow the same pattern as its corresponding skeleton joint hierarchy.
    for (const tinygltf::AnimationChannel& channel : animation.channels) {
      I32 node = channel.target_node;
      if (node != prevTarget) { 
        prevTarget = node;
        ++jointIndex;
      }
      tinygltf::Node& tnode = gltfModel->nodes[node];

      const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];
      
      const tinygltf::Accessor& inputAccessor = gltfModel->accessors[sampler.input];
      const tinygltf::BufferView& inputBufView = gltfModel->bufferViews[inputAccessor.bufferView];
      const R32* inputValues = reinterpret_cast<const R32*>(&gltfModel->buffers[inputBufView.buffer].data[inputAccessor.byteOffset + inputBufView.byteOffset]);
      // Read input data.
      // TODO():
    
      const tinygltf::Accessor& outputAccessor = gltfModel->accessors[sampler.output];
      const tinygltf::BufferView& outputBufView = gltfModel->bufferViews[outputAccessor.bufferView];
      const R32* outputValues = reinterpret_cast<const R32*>(&gltfModel->buffers[outputBufView.buffer].data[outputAccessor.byteOffset + outputBufView.byteOffset]);
      // Read output data.
      // TODO():
      if (clip->_aAnimPoseSamples.size() < inputAccessor.count) {
        std::map<R32, AnimPose> poses;
        for (auto& pose : clip->_aAnimPoseSamples) {
          poses[pose._time] = std::move(pose);
        } 
        clip->_aAnimPoseSamples.resize(inputAccessor.count); 
        for (size_t inputId = 0; inputId < inputAccessor.count; ++inputId) {
            R32 kt = inputValues[inputId];
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
          pose._aLocalPoses[jointIndex]._id = node;
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
          pose._aLocalPoses[jointIndex]._id = node;
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
          pose._aLocalPoses[jointIndex]._id = node;
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
            R32 weight = outputValues[outputId + n];
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
    AnimAssetManager::cache(clip->_name, clip);
  }

  return Model_Animated;
}


static void FlipStaticTrianglesInArray(std::vector<StaticVertex>& vertices)
{
  for (size_t i = 0, count = vertices.size(); i < count - 2; i += 3)
    std::swap(vertices[i], vertices[i + 2]);
}


static Mesh* LoadMesh(const tinygltf::Node& node,
                      U32 nodeIdx, 
                      const tinygltf::Model& model, 
                      Model* engineModel, 
                      Matrix4& localMatrix)
{
  Mesh* pMesh = nullptr;
  if (node.mesh > -1) {
    const tinygltf::Mesh& mesh = model.meshes[node.mesh];
    engineModel->nodeHierarchy[nodeIdx]._meshId = node.mesh;
    std::vector<Primitive> primitives;
    // Mesh Should hold the fully buffer data. Primitives specify start and index count, that
    // defines some submesh in the full mesh object.
    pMesh = new Mesh();
    
    std::vector<std::vector<MorphVertex> > morphVertices;
    std::vector<StaticVertex> vertices;
    std::vector<U32>          indices;
    Vector3                   min, max;
    CmdConfigBits             globalConfig = 0;

    if (!mesh.weights.empty()) {
      globalConfig |= CMD_MORPH_BIT;
      morphVertices.resize(mesh.weights.size());
    }

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
      const tinygltf::Primitive& primitive = mesh.primitives[i];
      Primitive primData;
      U32   vertexStart = static_cast<U32>(vertices.size());
      U32   indexStart = static_cast<U32>(indices.size());
      U32   indexCount = 0;
      if (primitive.indices < 0) continue;
      R_ASSERT(primitive.attributes.find("POSITION") != primitive.attributes.end(), "No position values within mesh!");

      {
        const R32* bufferPositions = nullptr;
        const R32* bufferNormals = nullptr;
        const R32* bufferTexCoords = nullptr;

        const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView& bufViewPos = model.bufferViews[positionAccessor.bufferView];
        bufferPositions =
          reinterpret_cast<const R32*>(&model.buffers[bufViewPos.buffer].data[positionAccessor.byteOffset + bufViewPos.byteOffset]);

        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
          const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
          const tinygltf::BufferView& bufViewNorm = model.bufferViews[normalAccessor.bufferView];
          bufferNormals =
            reinterpret_cast<const R32*>(&model.buffers[bufViewNorm.buffer].data[normalAccessor.byteOffset + bufViewNorm.byteOffset]);
          
        }

        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
          const tinygltf::Accessor& texcoordAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
          const tinygltf::BufferView& bufViewTexCoord0 = model.bufferViews[texcoordAccessor.bufferView];
          bufferTexCoords =
            reinterpret_cast<const R32*>(&model.buffers[bufViewTexCoord0.buffer].data[texcoordAccessor.byteOffset + bufViewTexCoord0.byteOffset]);
        }

        for (size_t value = 0; value < positionAccessor.count; ++value) {
          StaticVertex vertex;
          Vector3 p(&bufferPositions[value * 3]);
          vertex.position = Vector4(p, 1.0f) * localMatrix;
          vertex.position.w = 1.0f;
          vertex.normal = Vector4((Vector3(&bufferNormals[value * 3]) * Matrix3(localMatrix)).normalize(), 0.0f);
          vertex.texcoord0 = bufferTexCoords ? Vector2(&bufferTexCoords[value * 2]) : Vector2(0.0f, 0.0f);
          vertex.texcoord0.y = vertex.texcoord0.y > 1.0f ? vertex.texcoord0.y - 1.0f : vertex.texcoord0.y;
          vertex.texcoord1 = Vector2();
          //vertex.position.y *= -1.0f;
          //vertex.normal.y *= -1.0f;
          vertices.push_back(vertex);
          min = Vector3::minimum(min, p);
          max = Vector3::maximum(max, p);
          primData._aabb.min = Vector3::minimum(primData._aabb.min, p);
          primData._aabb.max = Vector3::maximum(primData._aabb.max, p);
        }
      }

      // Indices.
      {
        const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& iBufView = model.bufferViews[indAccessor.bufferView];
        const tinygltf::Buffer& iBuf = model.buffers[iBufView.buffer];
        indexCount = static_cast<U32>(indAccessor.count);

        // TODO(): In progress. 
        switch (indAccessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
          const U32* buf = (const U32*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(buf[index] + vertexStart);
          }
        } break;
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
          const U16* buf = (const U16*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(((U32)buf[index]) + vertexStart);
          }
        } break;
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
          const U8* buf = (const U8*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(((U32)buf[index]) + vertexStart);
          }
        } break;
        };
      }

      // Check for each for morph target. For each target, we push to their corresponding maps.
      if (!primitive.targets.empty()) {
        for (size_t mi = 0; mi < primitive.targets.size(); ++mi) {
          std::map<std::string, int>& target = 
            const_cast<std::map<std::string, int>&>(primitive.targets[mi]);
          const R32*  morphPositions = nullptr;
          const R32* morphNormals = nullptr;
          const R32* morphTexCoords = nullptr;            
  
          const tinygltf::Accessor& morphPositionAccessor = model.accessors[target["POSITION"]];
          const tinygltf::BufferView& morphPositionView = model.bufferViews[morphPositionAccessor.bufferView];
          morphPositions = reinterpret_cast<const R32*>(&model.buffers[morphPositionView.buffer].data[morphPositionView.byteOffset + morphPositionAccessor.byteOffset]);
         
          if (target.find("NORMAL") != target.end()) {
            const tinygltf::Accessor& morphNormalAccessor = model.accessors[target["NORMAL"]];
            const tinygltf::BufferView& morphNormalView = model.bufferViews[morphNormalAccessor.bufferView];
            morphNormals = reinterpret_cast<const R32*>(&model.buffers[morphNormalView.buffer].data[morphNormalAccessor.byteOffset + morphNormalView.byteOffset]);
          }
        
          if (target.find("TEXCOORD_0") != target.end()) {
            const tinygltf::Accessor& morphTexCoordAccessor = model.accessors[target["TEXCOORD_0"]];
            const tinygltf::BufferView& morphTexCoordView = model.bufferViews[morphTexCoordAccessor.bufferView];
            morphTexCoords = reinterpret_cast<const R32*>(&model.buffers[morphTexCoordView.buffer].data[morphTexCoordAccessor.byteOffset + morphTexCoordView.byteOffset]);
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

      GeneratePrimitive(primData, engineModel->materials[primitive.material], indexStart, indexCount);

      primData._aabb.computeCentroid();;

      primData._localConfigs |= globalConfig;
      if (engineModel->materials[primitive.material]->getNative()->isTransparent()) {
        primData._localConfigs |= CMD_TRANSPARENT_BIT;  
      }
      primitives.push_back(primData);
    }

    pMesh->initialize(&gRenderer(), vertices.size(), vertices.data(), Mesh::STATIC, indices.size(), indices.data());
    pMesh->setMin(min);
    pMesh->setMax(max);
    pMesh->updateAABB();
    std::string name = engineModel->name + "_mesh_" + mesh.name;
    MeshCache::cache(name, pMesh);
    engineModel->meshes.push_back(pMesh);
    for (auto& prim : primitives) {
      pMesh->pushPrimitive(prim);
    }
    pMesh->sortPrimitives(Mesh::TRANSPARENCY_LAST);


    if (!morphVertices.empty()) {
      pMesh->allocateMorphTargetBuffer(morphVertices.size());
      for ( size_t i = 0; i < morphVertices.size(); ++i ) {
        auto& verts = morphVertices[i];
        pMesh->initializeMorphTarget(&gRenderer(), i, verts.size(), verts.data(), sizeof(MorphVertex)); 
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
    std::vector<U32>          indices;
    Vector3                   min, max;
    CmdConfigBits             globalConfig = CMD_SKINNED_BIT;

    if (!mesh.weights.empty()) {
      globalConfig |= CMD_MORPH_BIT;
      morphVertices.resize(mesh.weights.size());
    }

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
      const tinygltf::Primitive& primitive = mesh.primitives[i];
      U32   vertexStart = static_cast<U32>(vertices.size());
      U32   indexStart = static_cast<U32>(indices.size());
      U32   indexCount = 0;
      Primitive primData;
      if (primitive.indices < 0) continue;
      R_ASSERT(primitive.attributes.find("POSITION") != primitive.attributes.end(), "No position values within mesh!");
      
      {
        const R32* bufferPositions = nullptr;
        const R32* bufferNormals = nullptr;
        const R32* bufferTexCoords = nullptr;
        const R32* bufferWeights = nullptr; 
        const U8* bufferJoints = nullptr;
        I32 jointType = -1;

        const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView& bufViewPos = model.bufferViews[positionAccessor.bufferView];
        bufferPositions =
          reinterpret_cast<const R32*>(&model.buffers[bufViewPos.buffer].data[positionAccessor.byteOffset + bufViewPos.byteOffset]);
        const std::vector<double>& dmin = positionAccessor.minValues;
        const std::vector<double>& dmax = positionAccessor.maxValues;

        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
          const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
          const tinygltf::BufferView& bufViewNorm = model.bufferViews[normalAccessor.bufferView];
          bufferNormals =
            reinterpret_cast<const R32*>(&model.buffers[bufViewNorm.buffer].data[normalAccessor.byteOffset + bufViewNorm.byteOffset]);
        }

        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
          const tinygltf::Accessor& texcoordAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
          const tinygltf::BufferView& bufViewTexCoord0 = model.bufferViews[texcoordAccessor.bufferView];
          bufferTexCoords =
            reinterpret_cast<const R32*>(&model.buffers[bufViewTexCoord0.buffer].data[texcoordAccessor.byteOffset + bufViewTexCoord0.byteOffset]);
        }

        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
          const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
          const tinygltf::BufferView& bufferViewJoints = model.bufferViews[jointAccessor.bufferView];
          bufferJoints = 
            reinterpret_cast<const U8*>(&model.buffers[bufferViewJoints.buffer].data[jointAccessor.byteOffset + bufferViewJoints.byteOffset]);
          jointType = jointAccessor.componentType;
        }

        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
          const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
          const tinygltf::BufferView& bufferViewWeight = model.bufferViews[weightAccessor.bufferView];
          bufferWeights =
            reinterpret_cast<const R32*>(&model.buffers[bufferViewWeight.buffer].data[weightAccessor.byteOffset + bufferViewWeight.byteOffset]);
        }

        for (size_t value = 0; value < positionAccessor.count; ++value) {
          SkinnedVertex vertex;
          null_bones(vertex);
          Vector3 p(&bufferPositions[value * 3]);
          vertex.position = Vector4(p, 1.0f) * localMatrix;
          vertex.position.w = 1.0f;
          vertex.normal = Vector4((Vector3(&bufferNormals[value * 3]) * Matrix3(localMatrix)).normalize(), 0.0f);
          vertex.texcoord0 = bufferTexCoords ? Vector2(&bufferTexCoords[value * 2]) : Vector2(0.0f, 0.0f);
          vertex.texcoord0.y = vertex.texcoord0.y > 1.0f ? vertex.texcoord0.y - 1.0f : vertex.texcoord0.y;
          vertex.texcoord1 = Vector2();
          if (bufferWeights && bufferJoints) {
            vertex.boneWeights = Vector4(&bufferWeights[value * 4]);
            switch (jointType) {
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
              {
                vertex.boneIds[0] = (I32)((U16*)bufferJoints)[value * 4 + 0];
                vertex.boneIds[1] = (I32)((U16*)bufferJoints)[value * 4 + 1];
                vertex.boneIds[2] = (I32)((U16*)bufferJoints)[value * 4 + 2];
                vertex.boneIds[3] = (I32)((U16*)bufferJoints)[value * 4 + 3];
              } break;
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
              default: 
              {
                vertex.boneIds[0] = (I32)bufferJoints[value * 4 + 0];
                vertex.boneIds[1] = (I32)bufferJoints[value * 4 + 1];
                vertex.boneIds[2] = (I32)bufferJoints[value * 4 + 2];
                vertex.boneIds[3] = (I32)bufferJoints[value * 4 + 3];
              }break;
            }
          }
          //vertex.position.y *= -1.0f;
          //vertex.normal.y *= -1.0f;
          vertices.push_back(vertex);
          min = Vector3::minimum(min, p);
          max = Vector3::maximum(max, p);
          primData._aabb.min = Vector3::minimum(primData._aabb.min, p);
          primData._aabb.max = Vector3::maximum(primData._aabb.max, p);
        }
      }

      // Indices.
      {
        const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& iBufView = model.bufferViews[indAccessor.bufferView];
        const tinygltf::Buffer& iBuf = model.buffers[iBufView.buffer];
        indexCount = static_cast<U32>(indAccessor.count);

        // TODO(): In progress. 
        switch (indAccessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
          const U32* buf = (const U32*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(buf[index] + vertexStart);
          }
        } break;
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
          const U16* buf = (const U16*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(((U32)buf[index]) + vertexStart);
          }
        } break;
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
          const U8* buf = (const U8*)&iBuf.data[indAccessor.byteOffset + iBufView.byteOffset];
          for (size_t index = 0; index < indAccessor.count; ++index) {
            indices.push_back(((U32)buf[index]) + vertexStart);
          }
        } break;
        };
      }

      // Check for each for morph target. For each target, we push to their corresponding maps.
      if (!primitive.targets.empty()) {
        for (size_t mi = 0; mi < primitive.targets.size(); ++mi) {
          std::map<std::string, int>& target =
            const_cast<std::map<std::string, int>&>(primitive.targets[mi]);
          const R32*  morphPositions = nullptr;
          const R32* morphNormals = nullptr;
          const R32* morphTexCoords = nullptr;

          const tinygltf::Accessor& morphPositionAccessor = model.accessors[target["POSITION"]];
          const tinygltf::BufferView& morphPositionView = model.bufferViews[morphPositionAccessor.bufferView];
          morphPositions = reinterpret_cast<const R32*>(&model.buffers[morphPositionView.buffer].data[morphPositionView.byteOffset + morphPositionAccessor.byteOffset]);

          if (target.find("NORMAL") != target.end()) {
            const tinygltf::Accessor& morphNormalAccessor = model.accessors[target["NORMAL"]];
            const tinygltf::BufferView& morphNormalView = model.bufferViews[morphNormalAccessor.bufferView];
            morphNormals = reinterpret_cast<const R32*>(&model.buffers[morphNormalView.buffer].data[morphNormalAccessor.byteOffset + morphNormalView.byteOffset]);
          }

          if (target.find("TEXCOORD_0") != target.end()) {
            const tinygltf::Accessor& morphTexCoordAccessor = model.accessors[target["TEXCOORD_0"]];
            const tinygltf::BufferView& morphTexCoordView = model.bufferViews[morphTexCoordAccessor.bufferView];
            morphTexCoords = reinterpret_cast<const R32*>(&model.buffers[morphTexCoordView.buffer].data[morphTexCoordAccessor.byteOffset + morphTexCoordView.byteOffset]);
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

      GeneratePrimitive(primData, engineModel->materials[primitive.material], indexStart, indexCount);

      primData._aabb.computeCentroid();

      primData._localConfigs |= globalConfig;
      if (engineModel->materials[primitive.material]->getNative()->isTransparent()) {
        primData._localConfigs |= CMD_TRANSPARENT_BIT;  
      }
      // TODO():
      //    Still need to add start and index count.
      primitives.push_back(primData);
    }

    for (size_t i = 0; i < mesh.targets.size(); ++i) {
      auto target = mesh.targets[i];
    }

    pMesh->initialize(&gRenderer(), vertices.size(), vertices.data(), Mesh::SKINNED, indices.size(), indices.data());
    pMesh->setMin(min);
    pMesh->setMax(max);
    pMesh->updateAABB();
    MeshCache::cache(mesh.name, pMesh);
    engineModel->meshes.push_back(pMesh);
    for (auto& primData : primitives) {
      pMesh->pushPrimitive(primData);
    }
    pMesh->sortPrimitives(Mesh::TRANSPARENCY_LAST);

    if (!morphVertices.empty()) {
      pMesh->allocateMorphTargetBuffer(morphVertices.size());
      for (size_t i = 0; i < morphVertices.size(); ++i) {
        auto& verts = morphVertices[i];
        pMesh->initializeMorphTarget(&gRenderer(), i, verts.size(), verts.data(), sizeof(MorphVertex));
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
    t = Vector3(static_cast<R32>(tnative[0]),
      static_cast<R32>(tnative[1]),
      static_cast<R32>(tnative[2]));
  }

  if (node.rotation.size() == 4) {
    const double* rq = node.rotation.data();
    r = Quaternion(static_cast<R32>(rq[0]),
      static_cast<R32>(rq[1]),
      static_cast<R32>(rq[2]),
      static_cast<R32>(rq[3]));
  }

  if (node.scale.size() == 3) {
    const double* sv = node.scale.data();
    s = Vector3(static_cast<R32>(sv[0]),
      static_cast<R32>(sv[1]),
      static_cast<R32>(sv[2]));
  }

  Matrix4 localMatrix = Matrix4::identity();
  if (node.matrix.size() == 16) {
    localMatrix = Matrix4(node.matrix.data());
  } else {
    Matrix4 T = Matrix4::translate(Matrix4::identity(), t);
    Matrix4 R = r.toMatrix4();
    Matrix4 S = Matrix4::scale(Matrix4::identity(), s);
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
  B32 rootInJoints = false;
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
  
  const R32* bindMatrices = reinterpret_cast<const R32*>(&buf.data[bufView.byteOffset + accessor.byteOffset]);  

  for (size_t i = 0; i < accessor.count; ++i) {
    Matrix4 invBindMat(&bindMatrices[i * 16]);
    Matrix4 bindMat = invBindMat.inverse();
    bindMat = bindMat * parentMatrix;
    skeleton._joints[i]._invBindPose = bindMat.inverse();
  }

  struct NodeTag {
    U8                _parent;
    Matrix4           _parentTransform;
  };

  std::map<I32, NodeTag> nodeMap;
  if (skin.skeleton != -1) {
    const tinygltf::Node& root = model.nodes[skin.skeleton];
    NodeTransform rootTransform = CalculateGlobalTransform(root, 
      Matrix4::scale(Matrix4(), Vector3(-1.0f, 1.0f, 1.0f)));
    skeleton._rootInvTransform = rootTransform._globalMatrix.inverse();
    NodeTag tag{ 0xff, Matrix4() };
    nodeMap[skin.skeleton] = tag;
    for (size_t i = 0; i < root.children.size(); ++i) {
      NodeTag tag = { (rootInJoints ? static_cast<U8>(0) : static_cast<U8>(0xff)), 
        rootTransform._globalMatrix };
      nodeMap[root.children[i]] = tag;
    }
  }

  for (size_t i = 0; i < skin.joints.size(); ++i) {
    size_t idx = i;
    Joint& joint = skeleton._joints[idx];
    I32 skinJointIdx = skin.joints[i];
    const tinygltf::Node& node = model.nodes[skinJointIdx];
    NodeTransform localTransform;

    auto it = nodeMap.find(skinJointIdx);
    if (it != nodeMap.end()) {
      NodeTag& tag = it->second;
      localTransform = CalculateGlobalTransform(node, tag._parentTransform);    
      joint._iParent = tag._parent;
    }

    joint._id = static_cast<U8>(skinJointIdx);
    for (size_t child = 0; child < node.children.size(); ++child) {
      NodeTag tag = { static_cast<U8>(idx), localTransform._globalMatrix };
      nodeMap[node.children[child]] = tag;
    }
  }
  
  Skeleton::pushSkeleton(skeleton);
  
  engineModel->skeletons.push_back(Skeleton::getSkeleton(skeleton._uuid));
  return skeleton._uuid;
}


static void LoadNode(const U32 nodeId, 
                     const tinygltf::Node& node, 
                     const tinygltf::Model& model, 
                     Model* engineModel, 
                     const Matrix4& parentMatrix, 
                     const R32 scale)
{
  NodeTransform transform = CalculateGlobalTransform(node, parentMatrix);
  //engineModel->nodeHierarchy[nodeId] = {};
  if (!node.children.empty()) {
    for (size_t i = 0; i < node.children.size(); ++i) {
      engineModel->nodeHierarchy[node.children[i]]._parentId = nodeId;
      engineModel->nodeHierarchy[node.children[i]]._meshId = Mesh::kMeshUnknownValue;
      LoadNode(node.children[i], 
               model.nodes[node.children[i]], 
               model, 
               engineModel, 
               transform._globalMatrix, 
               scale);
    }
  }

  if (node.skin != -1) {
    engineModel->nodeHierarchy[nodeId]._nodeConfig |= Model_Skinned;
    skeleton_uuid_t skeleId = LoadSkin(node, model, engineModel, transform._globalMatrix);
    Mesh* pMesh = LoadSkinnedMesh(node, model, engineModel, transform._globalMatrix);
    pMesh->setSkeletonReference(skeleId);
  } else {
    if (LoadMesh(node, 
                 nodeId, 
                 model, 
                 engineModel, 
                 transform._globalMatrix)) {
      engineModel->nodeHierarchy[nodeId]._nodeConfig |= Model_Mesh;
    }
  }
}


void GetFilenameAndType(const std::string& path, std::string& filenameOut, U32& typeOut)
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


ModelResultBits load(const std::string path)
{
  ModelResultBits result = 0;
  Model*           model = nullptr;
  static U64 copy = 0;
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  std::string err;  
  std::string modelName = "Unknown" + std::to_string(copy++);
  U32 type = 0;
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

  result |= LoadSamplers(&gltfModel, model);
  result |= LoadTextures(&gltfModel, model);
  result |= LoadMaterials(&gltfModel, model);

  tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    tinygltf::Node& node = gltfModel.nodes[scene.nodes[i]];
    Matrix4 mat = Matrix4::scale(Matrix4::identity(), Vector3(-1.0f, 1.0f, 1.0f));
    LoadNode(scene.nodes[i], node, gltfModel, model, mat, 1.0);
  }

  result |= LoadAnimations(&gltfModel, model);

  ModelCache::cache(model->name, model);
  result |= Model_Cached;

  result |= Model_Success;
  return result;
}
} // GLTF
} // ModelLoader
} // Recluse