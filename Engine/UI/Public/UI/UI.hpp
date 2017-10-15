// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"
#include "FontManager.hpp"



namespace Recluse {


class UI : public EngineModule<UI> {
public:

  void          OnStartUp() override;
  void          OnShutDown() override;

  void          UpdateState(r64 dt);

  // Print debug text onto the ui overlay.
  void          DebugPrint(std::string text, u32 width, u32 height);

private:
  std::vector<std::string>    debugTexts;
};


UI& gUI();
} // Recluse