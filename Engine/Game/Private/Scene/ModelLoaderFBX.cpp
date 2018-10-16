// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "ModelLoaderFBX.hpp"
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

#include <fbxsdk.h>
#include <fbxsdk/fileio/fbxiosettings.h>

namespace Recluse {
namespace ModelLoader {
namespace FBX {


std::map<std::string, Texture2D*> texturemap;
std::map<std::string, Material*>  materialmap;


FbxFileTexture* GetTextureFromProperty(FbxProperty* pProp)
{
  return (FbxFileTexture*)pProp->GetSrcObject(FbxCriteria::ObjectType(FbxFileTexture::ClassId));
}


Texture2D* ImportTexture(FbxTexture* fileTexture, Model* engineModel)
{
  if (!fileTexture) { return nullptr; }
  FbxFileTexture* xfileTexture = FbxCast<FbxFileTexture>(fileTexture);
  std::string filename = xfileTexture->GetFileName();
  auto it = texturemap.find(filename);
  if (it != texturemap.end()) {
    return it->second;
  }

  Image img;
  img.Load(filename.c_str());
  Texture2D* tex = gRenderer().CreateTexture2D();
  tex->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height());
  tex->Update(img);
  img.CleanUp();  

  TextureCache::Cache(tex);
  texturemap[filename] = tex;
  engineModel->textures.push_back(tex);
  return tex;
}


void LoadMaterials(FbxNode* node, Model* engineModel)
{
  for (i32 m = 0; m < node->GetMaterialCount(); ++m) {
    FbxSurfaceMaterial* surfaceMaterial = node->GetMaterial(m);
    std::string name = surfaceMaterial->GetName();
    {
      auto it = materialmap.find(name);
      if (it != materialmap.end()) {
        continue;
      }
    }
    Material* engineMat = new Material();
    engineMat->Initialize(&gRenderer());
    i32 layerIndex = 0;
    FbxProperty property;
    FBXSDK_FOR_EACH_TEXTURE(layerIndex) {
      const char* tChannelName = FbxLayerElement::sTextureChannelNames[layerIndex];
      property = surfaceMaterial->FindProperty(tChannelName);
      if (property.IsValid()) {
        i32 textureCount = property.GetSrcObjectCount<FbxTexture>();
        for (i32 t = 0; t < textureCount; ++t) {
          FbxTexture* lTexture = property.GetSrcObject<FbxTexture>(t);
          Texture2D* tex = ImportTexture(lTexture, engineModel);
          // TODO(): Figure out more texture handling.
          if (strcmp(tChannelName, "DiffuseColor") == 0) {
            engineMat->SetAlbedo(tex);
            engineMat->EnableAlbedo(true);
          }
        }
      }
    }
    
    MaterialCache::Cache(name, 
      engineMat);
    engineModel->materials.push_back(engineMat);
    materialmap[name] = engineMat;
  }
}


void LoadMesh(FbxNode* pNode, Model* engineModel)
{
  FbxMesh* pMesh = pNode->GetMesh();
  if (!pMesh) return;
  u32 numVertices = pMesh->GetControlPointsCount();
  u32 numPolygons = pMesh->GetPolygonCount();
  u32 uvElementCount = pMesh->GetElementUVCount();
  u32 materialCount = pMesh->GetElementMaterialCount();
  FbxStringList uvSetNamesList;
  std::vector<StaticVertex> vertices;//(numVertices);
  std::vector<Primitive>    primitives;
  std::vector<u32> indices;
  FbxVector4* fbxVertices = pMesh->GetControlPoints();
  Mesh* engineMesh = new Mesh();
  u32 vertexCounter = 0;
  pMesh->GetUVSetNames(uvSetNamesList);
  const FbxGeometryElementUV* uvElement = nullptr;
  for (u32 i = 0; i < uvSetNamesList.GetCount(); ++i) {
    const char* uvSetName = uvSetNamesList.GetStringAt(i);
    uvElement = pMesh->GetElementUV(uvSetName);
    if (!uvElement) continue;
    if (uvElement->GetMappingMode() != FbxGeometryElementUV::eByControlPoint &&
      uvElement->GetMappingMode() != FbxGeometryElementUV::eByPolygonVertex)
      continue;
    break;
  }

  for (u32 polyIdx = 0; polyIdx < numPolygons; ++polyIdx) {
    u32 vertCount = pMesh->GetPolygonSize(polyIdx);
    i32 startIdx = pMesh->GetPolygonVertexIndex(polyIdx);
    for (u32 vertIdx = 0; vertIdx < vertCount; ++vertIdx) {
      FbxVector4 normal;
      const i32 controlPointIdx = pMesh->GetPolygonVertex(polyIdx, vertIdx);
      pMesh->GetPolygonVertexNormal(polyIdx, vertIdx, normal);
      StaticVertex vertex;
      vertex.position = Vector4((r32)fbxVertices[controlPointIdx].mData[0],
                                (r32)fbxVertices[controlPointIdx].mData[1],
                                (r32)fbxVertices[controlPointIdx].mData[2],
                                1.0f);
      Matrix4 scale = Matrix4::Scale(Matrix4(), Vector3(-1.0f, 1.0f, 1.0f));
      vertex.position = vertex.position * scale;
      vertex.normal = Vector4((r32)normal.mData[0], 
                              (r32)normal.mData[1], 
                              (r32)normal.mData[2], 
                              1.0f);
      vertex.normal = Vector4(Vector3(vertex.normal.x, vertex.normal.y, vertex.normal.z) * Matrix3(scale), 1.0f);
      // get uvs.
      i32 directUVIdx = -1;
      if (uvElement->GetMappingMode() == FbxGeometryElementUV::eByControlPoint) {
        if (uvElement->GetReferenceMode() == FbxGeometryElementUV::eDirect) {
          directUVIdx = controlPointIdx;
        } else if (uvElement->GetReferenceMode() == FbxGeometryElementUV::eIndexToDirect) {
          directUVIdx = uvElement->GetIndexArray().GetAt(controlPointIdx);
        }
      } else if (uvElement->GetMappingMode() == FbxGeometryElementUV::eByPolygonVertex) {
        if (uvElement->GetReferenceMode() == FbxGeometryElementUV::eDirect || 
            uvElement->GetReferenceMode() == FbxGeometryElementUV::eIndexToDirect) {
          directUVIdx = pMesh->GetTextureUVIndex(polyIdx, vertIdx);
        }
      }
      if (directUVIdx != -1) {
        FbxVector2 uv = uvElement->GetDirectArray().GetAt(directUVIdx);
        vertex.texcoord0 = Vector2((r32)uv.mData[0],
                                    1.0f - (r32)uv.mData[1]);
      }

      vertices.push_back(vertex);
      indices.push_back(vertexCounter);
      ++vertexCounter;
    } 
  }

  i32 primidx = 0;
  for (i32 l = 0; l < materialCount; ++l) {
    FbxGeometryElementMaterial* leMat = pMesh->GetElementMaterial(l);
    if (!leMat) continue;
    i32 lMaterialCount = 0;
    FbxSurfaceMaterial* surfaceMaterial = pMesh->GetNode()->GetMaterial(leMat->GetIndexArray().GetAt(l));
    std::string name = surfaceMaterial->GetName();
    if (leMat->GetReferenceMode() == FbxGeometryElementMaterial::eDirect ||
        leMat->GetReferenceMode() == FbxGeometryElementMaterial::eIndexToDirect) {
      lMaterialCount = pMesh->GetNode()->GetMaterialCount();
    }
    if (leMat->GetReferenceMode() == FbxGeometryElementMaterial::eIndex ||
        leMat->GetReferenceMode() == FbxGeometryElementMaterial::eIndexToDirect) {
      Material* pMat = materialmap[name]; 
      Primitive prim;
      prim._firstIndex = 0;
      prim._indexCount = indices.size();
      prim._pMat = pMat;
      prim._localConfigs = 0;
      primitives.push_back(prim);
    }
  }

  engineMesh->Initialize(&gRenderer(), vertices.size(), vertices.data(), Mesh::STATIC, 
    indices.size(), indices.data());
  for (u32 i = 0; i < primitives.size(); ++i) {
    engineMesh->PushPrimitive(primitives[i]);
  }
  MeshCache::Cache(pMesh->GetName(), engineMesh);
  engineModel->meshes.push_back(engineMesh);
}


void LoadNode(FbxNode* pNode, Model* engineModel)
{
  if (!pNode) return;

  {
    FbxMesh* pMesh = pNode->GetMesh();
    if (pMesh) { 
      FbxGeometryConverter convert(pNode->GetFbxManager());
      convert.Triangulate(pNode->GetNodeAttribute(), true);
      LoadMaterials(pNode, engineModel);
      LoadMesh(pNode, engineModel);
    }
  }

  for (u32 i = 0; i < pNode->GetChildCount(); ++i) {
    LoadNode(pNode->GetChild(i), engineModel);
  }
}


void GetFilename(const std::string& path, std::string& filenameOut)
{
  size_t cutoff = path.find_last_of('/');
  if (cutoff != std::string::npos) {
    size_t removeExtId = path.find_last_of('.');
    if (removeExtId != std::string::npos) {
      filenameOut = std::move(path.substr(cutoff + 1, removeExtId - (cutoff + 1)));
    }
  }
}


ModelResultBits Load(const std::string filename)
{
  Model* engineModel = new Model();
  FbxManager* sdkManager = FbxManager::Create();
  FbxIOSettings* ios = FbxIOSettings::Create( sdkManager, IOSROOT);
  ios->SetBoolProp(IMP_FBX_MATERIAL, true);
  ios->SetBoolProp(IMP_FBX_TEXTURE, true);
  sdkManager->SetIOSettings(ios);
  FbxImporter* pImporter = FbxImporter::Create(sdkManager, "");
  
  bool success = pImporter->Initialize(filename.c_str(), -1, sdkManager->GetIOSettings());
  if (!success) {
    return Model_Fail;
  }

  std::string name = "";  
  GetFilename(filename, name);
  FbxScene* pScene = FbxScene::Create(sdkManager, name.c_str());
  engineModel->name = pScene->GetName();
  pImporter->Import(pScene);

  FbxNode* rootNode = pScene->GetRootNode();
  LoadNode(rootNode, engineModel);

  pImporter->Destroy();
  ios->Destroy();
  sdkManager->Destroy();
  ModelCache::Cache(engineModel->name, engineModel);
  return 0;
}
}
}
}