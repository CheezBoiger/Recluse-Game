// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"
#include "FontManager.hpp"



namespace Recluse {


class Texture2D;
class Renderer;

// User Interface manager.
class UI : public EngineModule<UI> {
public:

  void                      OnStartUp() override;
  void                      OnShutDown() override;

  // Update the state of the manager.
  void                      UpdateState(r64 dt);

  // Print debug text onto the ui overlay.
  void                      DebugPrint(std::string text, u32 x, u32 y, u32 width, u32 height);
  void                      PrintText(std::string text, u32 x, u32 y, u32 width, u32 height);
  void                      PrintTexture(Texture2D* texture, u32 x, u32 y, u32 width, u32 height);

private:
  std::vector<std::string>  debugTexts;
};


UI& gUI();
} // Recluse