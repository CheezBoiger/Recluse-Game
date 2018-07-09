// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"
#include "FontManager.hpp"



namespace Recluse {


class Texture2D;
class Renderer;

// User Interface manager.
class UI : public EngineModule<UI> {
public:
  UI()
    : m_currForeColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f))
    , m_currBackColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f)) { }

  void                      OnStartUp() override;
  void                      OnShutDown() override;

  // Update the state of the manager.
  void                      UpdateState(r64 dt);


  void                      SetForegroundColor(const Vector4& color) { m_currForeColor = color; }
  void                      SetBackgroundColor(const Vector4& color) { m_currBackColor = color; }
  void                      EmitText(const std::string& text, r32 x, r32 y, r32 width, r32 height);

private:
  
  Vector4                 m_currForeColor;
  Vector4                 m_currBackColor;
};


UI& gUI();
} // Recluse