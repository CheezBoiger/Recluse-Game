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
  img.Load(FROM_TEXTURES_DIR("Box/albedo.jpg"));
  Texture2D* albedo = gRenderer().CreateTexture2D();
  albedo->Initialize(img.Width(), img.Height());
  albedo->_Name = "BoxAlbedo";
  albedo->Update(img);
  img.CleanUp();
  TextureCache::Cache(albedo);

  img.Load(FROM_TEXTURES_DIR("Box/normal.jpg"));
  Texture2D* normal = gRenderer().CreateTexture2D();
  normal->Initialize(img.Width(), img.Height());
  normal->_Name = "BoxNormal";
  normal->Update(img);
  img.CleanUp();
  TextureCache::Cache(normal);

  img.Load(FROM_TEXTURES_DIR("Box/emissive.jpg"));
  Texture2D* emissive = gRenderer().CreateTexture2D();
  emissive->Initialize(img.Width(), img.Height());
  emissive->_Name = "BoxEmissive";
  emissive->Update(img);
  img.CleanUp();
  TextureCache::Cache(emissive);

  img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_basecolor.png"));
  Texture2D* rustBase = gRenderer().CreateTexture2D();
  rustBase->Initialize(img.Width(), img.Height());
  rustBase->_Name = "RustedAlbedo";
  rustBase->Update(img);
  img.CleanUp();
  TextureCache::Cache(rustBase);

  img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_normal.png"));
  Texture2D* rustNormal = gRenderer().CreateTexture2D();
  rustNormal->Initialize(img.Width(), img.Height());
  rustNormal->_Name = "RustedNormal";
  rustNormal->Update(img);
  img.CleanUp();
  TextureCache::Cache(rustNormal);

  img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_roughness.png"));
  Texture2D* rustRough = gRenderer().CreateTexture2D();
  rustRough->Initialize(img.Width(), img.Height());
  rustRough->_Name = "RustedRough";
  rustRough->Update(img);
  img.CleanUp();
  TextureCache::Cache(rustRough);

  img.Load(FROM_TEXTURES_DIR("Sphere/rustediron2_metallic.png"));
  Texture2D* rustMetal = gRenderer().CreateTexture2D();
  rustMetal->Initialize(img.Width(), img.Height());
  rustMetal->_Name = "RustedMetal";
  rustMetal->Update(img);
  img.CleanUp();
  TextureCache::Cache(rustMetal);
}


void TextureCleanUp()
{
  TextureCache::CleanUpAll();
}