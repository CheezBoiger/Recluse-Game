// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "UI.hpp"
#include "GUIImage.hpp"

#include "Core/Win32/Mouse.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/RenderCmd.hpp"

#include "Core/Exception.hpp"


namespace Recluse {


UI& gUI()
{
  return UI::instance();
}


void UI::onStartUp( )
{
  gFontManager().startUp();
  if ( gRenderer().isActive( ) ) {
    m_currUiBuffer = gRenderer().getUiBuffer();
  }
}


void UI::onShutDown()
{
  gFontManager().shutDown();
  
}


void UI::updateState(r64 dt)
{
  Window* pWindow = gRenderer().getWindowRef();
  // Check if window. If none, return.
  if ( !pWindow )
  {
    return;
  }

  r32 mX = static_cast< r32 >( Mouse::getX( ) );
  r32 mY = static_cast< r32 >( Mouse::getY( ) );

  for ( auto& eventHandle : m_eventHandles )
  {
    Vector4 bb ( eventHandle.second._bbox._x,
                 eventHandle.second._bbox._y,
                 eventHandle.second._bbox._w,
                 eventHandle.second._bbox._h );
    if ( mX >= bb.x && 
         mX <= bb.z &&
         mY >= bb.y && 
         mY <= bb.w )
    {
      if ( eventHandle.second._eventOnHover )
      {
        eventHandle.second._eventOnHover( );
      }

      if ( Mouse::buttonDown( Mouse::ButtonType::LEFT ) &&
           eventHandle.second._eventOnPress )
      {
        eventHandle.second._eventOnPress( );
      }
    }
  }
}


void UI::EmitText( const std::string& text, r32 x, r32 y, r32 width, r32 height )
{
  UiText textCmd;
  memcpy( textCmd._str, text.c_str( ), text.size( ) );

  textCmd._sz = text.size();
  textCmd._bgColor.r = static_cast< i32 >( m_currBackColor.x * 255.0f );
  textCmd._bgColor.g = static_cast< i32 >( m_currBackColor.y * 255.0f );
  textCmd._bgColor.b = static_cast< i32 >( m_currBackColor.z * 255.0f );
  textCmd._bgColor.a = static_cast< i32 >( m_currBackColor.w * 255.0f );

  textCmd._fgColor.r = static_cast< i32 >( m_currForeColor.x * 255.0f );
  textCmd._fgColor.g = static_cast< i32 >( m_currForeColor.y * 255.0f );
  textCmd._fgColor.b = static_cast< i32 >( m_currForeColor.z * 255.0f );
  textCmd._fgColor.a = static_cast< i32 >( m_currForeColor.w * 255.0f );

  textCmd._x = m_currX + x;
  textCmd._y = m_currY + y;
  textCmd._width = width;
  textCmd._height = height;

  m_currUiBuffer->EmitText( textCmd );
}


void UI::BeginCanvas(const std::string& title, r32 x, r32 y, r32 width, r32 height)
{
  UiBeginCanvasInfo begin;
  begin._backgroundColor = Color4(255, 255, 255, 0);
  begin._canvasBorderColor = Color4(255, 255, 255, 0);
  begin._fixedBackgroundColor = Color4(255, 255, 255, 0);
  begin._headerColor = Color4(255, 255, 255, 0);
  memcpy(begin._str, title.c_str(), title.size() + 1);
  begin._str[title.size()] = '\0';
  begin._x = x;
  begin._y = y;
  begin._width = width;
  begin._height = height;
  m_currUiBuffer->BeginCanvas(begin);
  m_currX = x;
  m_currY = y;
}


void UI::EndCanvas()
{
  m_currUiBuffer->EndCanvas();
}


void UI::EmitImage(GUIImage* image)
{
}
} // Recluse