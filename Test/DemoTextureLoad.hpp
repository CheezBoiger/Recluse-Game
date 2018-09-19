// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once

#include "Game/Engine.hpp"
#define ASSETS_DIR "Assets"
#define TEXTURES_DIR ASSETS_DIR##"/Textures"
#define FROM_TEXTURES_DIR(relative_path) TEXTURES_DIR##"/"##relative_path

using namespace Recluse;


void LoadTextures()
{
  Image img;
  {
    img.Load(FROM_TEXTURES_DIR("Box/albedo.jpg"));
    Texture2D* albedo = gRenderer().CreateTexture2D();
    albedo->Initialize(img.Width(), img.Height());
    albedo->_Name = "BoxAlbedo";
    albedo->Update(img);
    img.CleanUp();
    TextureCache::Cache(albedo);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Box/normal.jpg"));
    Texture2D* normal = gRenderer().CreateTexture2D();
    normal->Initialize(img.Width(), img.Height());
    normal->_Name = "BoxNormal";
    normal->Update(img);
    img.CleanUp();
    TextureCache::Cache(normal);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Box/emissive.jpg"));
    Texture2D* emissive = gRenderer().CreateTexture2D();
    emissive->Initialize(img.Width(), img.Height());
    emissive->_Name = "BoxEmissive";
    emissive->Update(img);
    img.CleanUp();
    TextureCache::Cache(emissive);
  }
  {
    img.Load(FROM_TEXTURES_DIR("grass1-albedo3.png"));
    Texture2D* rustBase = gRenderer().CreateTexture2D();
    rustBase->Initialize(img.Width(), img.Height());
    rustBase->_Name = "GrassyAlbedo";
    rustBase->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustBase);
  }
  {
    img.Load(FROM_TEXTURES_DIR("grass1-normal2.png"));
    Texture2D* rustNormal = gRenderer().CreateTexture2D();
    rustNormal->Initialize(img.Width(), img.Height());
    rustNormal->_Name = "GrassyNormal";
    rustNormal->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustNormal);
  }
  {
    img.Load(FROM_TEXTURES_DIR("grass1-rough.png"));
    Texture2D* rustRough = gRenderer().CreateTexture2D();
    rustRough->Initialize(img.Width(), img.Height());
    rustRough->_Name = "GrassyRough";
    rustRough->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustRough);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_basecolor.png"));
    Texture2D* rustMetal = gRenderer().CreateTexture2D();
    rustMetal->Initialize(img.Width(), img.Height());
    rustMetal->_Name = "RustedAlbedo";
    rustMetal->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustMetal);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_roughness.png"));
    Texture2D* rustMetal = gRenderer().CreateTexture2D();
    rustMetal->Initialize(img.Width(), img.Height());
    rustMetal->_Name = "RustedRough";
    rustMetal->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustMetal);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_normal.png"));
    Texture2D* rustMetal = gRenderer().CreateTexture2D();
    rustMetal->Initialize(img.Width(), img.Height());
    rustMetal->_Name = "RustedNormal";
    rustMetal->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustMetal);
  }
  {
    img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_metallic.png"));
    Texture2D* rustMetal = gRenderer().CreateTexture2D();
    rustMetal->Initialize(img.Width(), img.Height());
    rustMetal->_Name = "RustedMetal";
    rustMetal->Update(img);
    img.CleanUp();
    TextureCache::Cache(rustMetal);
  }
}


void LoadMaterials()
{
  {
    Material* material = new Material();
    material->Initialize(&gRenderer());
    Texture2D* tex;
    TextureCache::Get(RTEXT("GrassyAlbedo"), &tex);

    material->SetAlbedo(tex);
    material->SetBaseColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    material->EnableAlbedo(true);
    material->SetRoughnessFactor(1.0f);
    material->SetMetallicFactor(1.0f);
    TextureCache::Get(RTEXT("GrassyNormal"), &tex);
    material->SetNormal(tex);
    material->EnableNormal(true);

    TextureCache::Get(RTEXT("GrassyRough"), &tex);
    material->SetRoughnessMetallic(tex);
    material->EnableRoughness(true);
    MaterialCache::Cache(TEXT("GrassySample"), material);
  }

  {
    Material* material = new Material();
    material->Initialize(&gRenderer());
    Texture2D* tex;
    TextureCache::Get(RTEXT("RustedAlbedo"), &tex);

    material->SetAlbedo(tex);
    material->SetBaseColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    material->EnableAlbedo(true);
    material->SetRoughnessFactor(1.0f);
    material->SetMetallicFactor(1.0f);
    TextureCache::Get(RTEXT("RustedNormal"), &tex);
    material->SetNormal(tex);
    material->EnableNormal(true);

    TextureCache::Get(RTEXT("RustedRough"), &tex);
    material->SetRoughnessMetallic(tex);
    material->EnableRoughness(true);
    MaterialCache::Cache(TEXT("RustedSample"), material);  
  }
}