// Copyright (c) 2017 Recluse Project. All rights reserved.
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


void UI::UpdateState(r64 dt)
{
}
} // Recluse