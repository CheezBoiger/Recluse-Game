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

private:
};


UI& gUI();
} // Recluse