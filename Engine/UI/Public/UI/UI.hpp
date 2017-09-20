// Copyright (c) 2017 Recluse Project.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "FontManager.hpp"



namespace Recluse {


class UI : public EngineModule<UI> {
public:

  void          OnStartUp() override;
  void          OnShutDown() override;

  void          UpdateState();
};


UI& gUI();
} // Recluse