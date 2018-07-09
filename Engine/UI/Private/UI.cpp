// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "UI.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/RenderCmd.hpp"
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


void UI::EmitText(const std::string& text, r32 x, r32 y, r32 width, r32 height)
{
  UiText textCmd;
  memcpy(textCmd._str, text.c_str(), text.size());

  textCmd._sz = text.size();
  textCmd._bgColor[0] = static_cast<i32>(m_currBackColor.x * 255.0f);
  textCmd._bgColor[1] = static_cast<i32>(m_currBackColor.y * 255.0f);
  textCmd._bgColor[2] = static_cast<i32>(m_currBackColor.z * 255.0f);
  textCmd._bgColor[3] = static_cast<i32>(m_currBackColor.w * 255.0f);

  textCmd._fgColor[0] = static_cast<i32>(m_currForeColor.x * 255.0f);
  textCmd._fgColor[1] = static_cast<i32>(m_currForeColor.y * 255.0f);
  textCmd._fgColor[2] = static_cast<i32>(m_currForeColor.z * 255.0f);
  textCmd._fgColor[3] = static_cast<i32>(m_currForeColor.w * 255.0f);

  textCmd._x = x;
  textCmd._y = y;
  textCmd._width = width;
  textCmd._height = height;

  gRenderer().PushUiRender(textCmd);
}
} // Recluse