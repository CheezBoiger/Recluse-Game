// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Game/Engine.hpp"
#define ASSETS_DIR "Assets"
#define TEXTURES_DIR ASSETS_DIR##"/Textures"
#define FROM_TEXTURES_DIR(relative_path) TEXTURES_DIR##"/"##relative_path

using namespace Recluse;


void LoadMaterials()
{
  Image img;
  Texture2D* albedo = nullptr;
  Texture2D* normal = nullptr;
  Texture2D* emissive = nullptr;
  Texture2D* rustNormal = nullptr;
  Texture2D* rustRough = nullptr;
  Texture2D* rustAlbedo = nullptr;
  Texture2D* grassBase = nullptr;
  Texture2D* grassNormal = nullptr;
  Texture2D* grassRough = nullptr;
  {
    img.Load(FROM_TEXTURES_DIR("Box/albedo.jpg"));
    albedo = gRenderer().CreateTexture2D();
    albedo->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height());
    albedo->_Name = "BoxAlbedo";
    albedo->Update(img);
    img.CleanUp();
    TextureCache::Cache(albedo);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Box/normal.jpg"));
    normal = gRenderer().CreateTexture2D();
    normal->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height());
    normal->_Name = "BoxNormal";
    normal->Update(img);
    img.CleanUp();
    TextureCache::Cache(normal);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Box/emissive.jpg"));
    emissive = gRenderer().CreateTexture2D();
    emissive->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height());
    emissive->_Name = "BoxEmissive";
    emissive->Update(img);
    img.CleanUp();
    TextureCache::Cache(emissive);
  }
  {
    img.Load(FROM_TEXTURES_DIR("grass1-albedo3.png"));
    grassBase = gRenderer().CreateTexture2D();
    grassBase->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height());
    grassBase->_Name = "GrassyAlbedo";
    grassBase->Update(img);
    img.CleanUp();
    TextureCache::Cache(grassBase);
  }
  {
    img.Load(FROM_TEXTURES_DIR("grass1-normal2.png"));
    grassNormal = gRenderer().CreateTexture2D();
    grassNormal->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height());
    grassNormal->_Name = "GrassyNormal";
    grassNormal->Update(img);
    img.CleanUp();
    TextureCache::Cache(grassNormal);
  }
  {
    img.Load(FROM_TEXTURES_DIR("grass1-rough.png"));
    grassRough = gRenderer().CreateTexture2D();
    grassRough->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height());
    grassRough->_Name = "GrassyRough";
    grassRough->Update(img);
    img.CleanUp();
    TextureCache::Cache(grassRough);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_basecolor.png"));
    rustAlbedo = gRenderer().CreateTexture2D();
    rustAlbedo->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height(), true);
    rustAlbedo->_Name = "RustedAlbedo";
    rustAlbedo->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustAlbedo);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_roughness.png"));
    rustRough = gRenderer().CreateTexture2D();
    rustRough->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height(), true);
    rustRough->_Name = "RustedRough";
    rustRough->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustRough);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_normal.png"));
    rustNormal = gRenderer().CreateTexture2D();
    rustNormal->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height(), true);
    rustNormal->_Name = "RustedNormal";
    rustNormal->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustNormal);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_metallic.png"));
    Texture2D* rustMetal = gRenderer().CreateTexture2D();
    rustMetal->Initialize(RFORMAT_R8G8B8A8_UNORM, img.Width(), img.Height(), true);
    rustMetal->_Name = "RustedMetal";
    rustMetal->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustMetal);
  }
  {
    Material* material = new Material();
    material->Initialize(&gRenderer());
    material->SetAlbedo(grassBase);
    material->SetBaseColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    material->EnableAlbedo(true);
    material->SetRoughnessFactor(1.0f);
    material->SetMetallicFactor(1.0f);
    material->SetNormal(grassNormal);
    material->EnableNormal(true);

    material->SetRoughnessMetallic(grassRough);
    material->EnableRoughness(true);
    MaterialCache::Cache(TEXT("GrassySample"), material);
  }

  {
    Material* material = new Material();
    material->Initialize(&gRenderer());

    material->SetAlbedo(rustAlbedo);
    material->SetBaseColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    material->EnableAlbedo(true);
    material->SetRoughnessFactor(1.0f);
    material->SetMetallicFactor(1.0f);
    material->SetNormal(rustNormal);
    material->EnableNormal(true);

    material->SetRoughnessMetallic(rustRough);
    material->EnableRoughness(true);
    MaterialCache::Cache(TEXT("RustedSample"), material);  
  }
}