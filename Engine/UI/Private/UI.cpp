// Copyright (c) 2017 Recluse Project.
#include "UI.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


UI& gUI()
{
  return UI::Instance();
}


void UI::OnStartUp()
{
  gFontManager().StartUp();
}


void UI::OnShutDown()
{
  gFontManager().ShutDown();
}


void UI::UpdateState()
{
}
} // Recluse