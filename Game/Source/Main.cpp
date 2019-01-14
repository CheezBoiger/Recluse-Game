// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Game/Engine.hpp"

#include <string>
#include <cctype>

using namespace Recluse;


std::string GetOption(const std::string& line)
{
  size_t pos = line.find('=');
  if (pos == std::string::npos) return "";
  std::string option = line.substr(pos + 1);
  option.erase(std::remove_if(option.begin(), option.end(), [](u8 x) -> i32 { return std::isspace(x); }), option.end());
  std::transform(option.begin(), option.end(), option.begin(), std::tolower);
  return option;
}


b32 AvailableOption(const std::string& line, const tchar* option)
{
  size_t pos = line.find(option);
  if (pos != std::string::npos) return true;
  return false;
}


GraphicsConfigParams ReadGraphicsConfig(u32& w, u32& h)
{
  GraphicsConfigParams graphics = kDefaultGpuConfigs;
  FileHandle Buf;
  FilesystemResult result = gFilesystem().ReadFrom("Configs/RendererConfigs.recluse", &Buf);
  if (result == FilesystemResult_NotFound) {
    Log(rWarning) << "RendererConfigs not found! Setting default rendering configuration.\n";
    return graphics;
  }

  std::string line = "";
  for (size_t i = 0; i < Buf.Sz; ++i) {
    tchar ch = Buf.Buf[i];
    line.push_back(ch);
    if (ch == '\n') {
      std::cout << line;
      if (AvailableOption(line, "Buffering")) {
        std::string option = GetOption(line);
        if (option.compare("triple") == 0) {
          graphics._Buffering = TRIPLE_BUFFER;
        }
        else if (option.compare("single") == 0) {
          graphics._Buffering = SINGLE_BUFFER;
        }
        else {
          graphics._Buffering = DOUBLE_BUFFER;
        }
      }
      if (AvailableOption(line, "AntiAliasing")) {
        std::string option = GetOption(line);
        if (option.compare("fxaa") == 0) {
          graphics._AA = AA_FXAA_2x;
        }
        else if (option.compare("smaa2x") == 0) {
          graphics._AA = AA_SMAA_2x;
        }
        else {
          graphics._AA = AA_None;
        }
      }
      if (AvailableOption(line, "TextureQuality")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "ShadowQuality")) {
        std::string option = GetOption(line);
        if (option.compare("ultra") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_ULTRA;
        }
        else if (option.compare("high") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_HIGH;
        }
        else if (option.compare("medium") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_MEDIUM;
        }
        else if (option.compare("low") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_LOW;
        }
        else {
          graphics._Shadows = GRAPHICS_QUALITY_NONE;
        }
      }
      if (AvailableOption(line, "LightingQuality")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "ModelQuality")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "LevelOfDetail")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "RenderScale")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "VSync")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableVsync = true;
        }
        else {
          graphics._EnableVsync = false;
        }
      }
      if (AvailableOption(line, "ChromaticAberration")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableChromaticAberration = true;
        }
        else {
          graphics._EnableChromaticAberration = false;
        }
      }
      if (AvailableOption(line, "Bloom")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableBloom = true;
        }
        else {
          graphics._EnableBloom = false;
        }
      }
      if (AvailableOption(line, "PostProcessing")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnablePostProcessing = true;
        }
        else {
          graphics._EnablePostProcessing = false;
        }
      }
      if (AvailableOption(line, "SoftShadows")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableSoftShadows = true;
        }
        else {
          graphics._EnableSoftShadows = false;
        }
      }
      if (AvailableOption(line, "Multithreading")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableMultithreadedRendering = true;
        }
        else {
          graphics._EnableMultithreadedRendering = false;
        }
      }
      if (AvailableOption(line, "Resolution")) {
        std::string option = GetOption(line);
        if (option.compare("800x600") == 0) { graphics._Resolution = Resolution_800x600; w = 800; h = 600; }
        else if (option.compare("1200x800") == 0) { graphics._Resolution = Resolution_1200x800; w = 1200; h = 800; }
        else if (option.compare("1280x720") == 0) { graphics._Resolution = Resolution_1280x720; w = 1280; h = 720; }
        else if (option.compare("1440x900") == 0) { graphics._Resolution = Resolution_1440x900; w = 1400; h = 900; }
        else if (option.compare("1920x1080") == 0) { graphics._Resolution = Resolution_1920x1080; w = 1920; h = 1080; }
        else if (option.compare("1920x1440") == 0) { graphics._Resolution = Resolution_1920x1440; w = 1920; h = 1440; }
      }
      if (AvailableOption(line, "Window")) {
        std::string option = GetOption(line);
        if (option.compare("borderless") == 0) { graphics._WindowType = WindowType_Borderless; }
        else if (option.compare("fullscreen") == 0) { graphics._WindowType = WindowType_Fullscreen; }
        else if (option.compare("border") == 0) { graphics._WindowType = WindowType_Border; }
      }
      line.clear();
    }
  }
  return graphics;
}

int main(int c, char* argv[])
{
  // TODO():
  // Before the game engine starts up, we want to read our configuration file, 
  // used to determine engine settings saved by user.
  {
    u32 width = 800;
    u32 height = 600;
    GraphicsConfigParams params = ReadGraphicsConfig(width, height);
    gEngine().StartUp("Recluse", false, width, height, &params);
    gEngine().Run();
    Window* pWindow = gEngine().GetWindow();
    switch (params._WindowType) {
    case WindowType_Borderless:
    {
      pWindow->SetToWindowed(width, height, true);
      pWindow->SetToCenter();
    } break;
    case WindowType_Border:
    {
      pWindow->SetToWindowed(width, height);
      pWindow->SetToCenter();
    } break;
    case WindowType_Fullscreen:
    default:
      pWindow->SetToFullScreen();
    }
    pWindow->Show();
  }
  
  while (gEngine().Running()) {
    Time::Update();
    gEngine().ProcessInput();
    // Scene updating goes here.

    gEngine().Update();
  }

  AssetManager::CleanUpAssets();
  gEngine().CleanUp();
  return 0;
}