// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"
#include "FontManager.hpp"

#include <functional>
#include <map>

namespace Recluse {


class Texture2D;
class Renderer;
class GUIImage;
class BufferUI;

// User Interface manager.
class UI : public EngineModule<UI> {
public:

  typedef std::function<b32()> CallbackUI;

  UI()
    : m_currForeColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f))
    , m_currBackColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f))
    , m_currUiBuffer(nullptr)
    , m_currX(0.0f)
    , m_currY(0.0f) { }

  void                      onStartUp() override;
  void                      onShutDown() override;

  // Update the state of the manager.
  void                      updateState(r64 dt);


  // TODO(): Needs to be more object oriented in the API level, instead of this way.
  void                      SetForegroundColor(const Vector4& color) { m_currForeColor = color; }
  void                      SetBackgroundColor(const Vector4& color) { m_currBackColor = color; }
  
  void                      ImportFont(const std::string& fontpath);

  void                      BeginCanvas(const std::string& title, r32 x, r32 y, r32 width, r32 height);
  void                      EmitText(const std::string& text, r32 x, r32 y, r32 width, r32 height);
  void                      EndCanvas();
  void                      EmitImage(GUIImage* image);

  void SetEventHandle( const std::string& eventName, 
                       r32 x, 
                       r32 y, 
                       r32 w, 
                       r32 h, 
                       CallbackUI callbackOnPress = nullptr,
                       CallbackUI callabckOnHover = nullptr ) 
  {
    m_eventHandles[ eventName ] = { { x, y, w, h }, callbackOnPress, callabckOnHover };
  }

private:

  struct UIEvent 
  {
    struct 
    {
      r32 _x,
          _y,
          _w,
          _h;
    } _bbox;
    CallbackUI _eventOnPress;
    CallbackUI _eventOnHover;
    CallbackUI _eventOnHold;
    CallbackUI _eventOnRelease;
  };

  r32                     m_currX;
  r32                     m_currY;
  Vector4                 m_currForeColor;
  Vector4                 m_currBackColor;
  BufferUI*               m_currUiBuffer;
  std::map<std::string, UIEvent> m_eventHandles;
};


UI& gUI();
} // Recluse