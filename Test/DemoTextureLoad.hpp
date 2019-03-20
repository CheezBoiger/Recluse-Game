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
/*
  {
    img.load(FROM_TEXTURES_DIR("Box/albedo.jpg"));
    albedo = gRenderer().createTexture2D();
    albedo->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight());
    albedo->_Name = "BoxAlbedo";
    albedo->Update(img);
    img.cleanUp();
    TextureCache::cache(albedo);
  }
  {
    img.load(FROM_TEXTURES_DIR("Box/normal.jpg"));
    normal = gRenderer().createTexture2D();
    normal->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight());
    normal->_Name = "BoxNormal";
    normal->Update(img);
    img.cleanUp();
    TextureCache::cache(normal);
  }
  {
    img.load(FROM_TEXTURES_DIR("Box/emissive.jpg"));
    emissive = gRenderer().createTexture2D();
    emissive->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight());
    emissive->_Name = "BoxEmissive";
    emissive->Update(img);
    img.cleanUp();
    TextureCache::cache(emissive);
  }
  {
    img.load(FROM_TEXTURES_DIR("grass1-albedo3.png"));
    grassBase = gRenderer().createTexture2D();
    grassBase->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight());
    grassBase->_Name = "GrassyAlbedo";
    grassBase->Update(img);
    img.cleanUp();
    TextureCache::cache(grassBase);
  }
  {
    img.load(FROM_TEXTURES_DIR("grass1-normal2.png"));
    grassNormal = gRenderer().createTexture2D();
    grassNormal->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight());
    grassNormal->_Name = "GrassyNormal";
    grassNormal->Update(img);
    img.cleanUp();
    TextureCache::cache(grassNormal);
  }
  {
    img.load(FROM_TEXTURES_DIR("grass1-rough.png"));
    grassRough = gRenderer().createTexture2D();
    grassRough->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight());
    grassRough->_Name = "GrassyRough";
    grassRough->Update(img);
    img.cleanUp();
    TextureCache::cache(grassRough);
  }
*/
  {
    img.load(FROM_TEXTURES_DIR("Sphere/rustediron2_basecolor.png"));
    rustAlbedo = gRenderer().createTexture2D();
    rustAlbedo->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight(), true);
    rustAlbedo->_Name = "RustedAlbedo";
    rustAlbedo->update(img);
    img.cleanUp();
    TextureCache::cache(rustAlbedo);
  }
  {
    img.load(FROM_TEXTURES_DIR("Sphere/rustediron2_roughness.png"));
    rustRough = gRenderer().createTexture2D();
    rustRough->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight(), true);
    rustRough->_Name = "RustedRough";
    rustRough->update(img);
    img.cleanUp();
    TextureCache::cache(rustRough);
  }
  {
    img.load(FROM_TEXTURES_DIR("Sphere/rustediron2_normal.png"));
    rustNormal = gRenderer().createTexture2D();
    rustNormal->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight(), true);
    rustNormal->_Name = "RustedNormal";
    rustNormal->update(img);
    img.cleanUp();
    TextureCache::cache(rustNormal);
  }
  {
    img.load(FROM_TEXTURES_DIR("Sphere/rustediron2_metallic.png"));
    Texture2D* rustMetal = gRenderer().createTexture2D();
    rustMetal->initialize(RFORMAT_R8G8B8A8_UNORM, img.getWidth(), img.getHeight(), true);
    rustMetal->_Name = "RustedMetal";
    rustMetal->update(img);
    img.cleanUp();
    TextureCache::cache(rustMetal);
  }
/*
  {
    Material* material = new Material();
    material->initialize(&gRenderer());
    material->setAlbedo(grassBase);
    material->setBaseColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    material->enableAlbedo(true);
    material->setRoughnessFactor(1.0f);
    material->setMetallicFactor(1.0f);
    material->setNormal(grassNormal);
    material->enableNormal(true);

    material->setRoughnessMetallic(grassRough);
    material->enableRoughness(true);
    MaterialCache::cache(TEXT("GrassySample"), material);
  }
*/
  {
    Material* material = new Material();
    material->initialize(&gRenderer());

    material->setAlbedo(rustAlbedo);
    material->setBaseColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    material->enableAlbedo(true);
    material->setRoughnessFactor(1.0f);
    material->setMetallicFactor(1.0f);
    material->setNormal(rustNormal);
    material->enableNormal(true);

    material->setRoughnessMetallic(rustRough);
    material->enableRoughness(true);
    MaterialCache::cache(TEXT("RustedSample"), material);  
  }
}