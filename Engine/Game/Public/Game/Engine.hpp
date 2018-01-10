// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Core.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Win32/Keyboard.hpp"
#include "Core/Math/Quaternion.hpp"

#include "Geometry/Cube.hpp"
#include "Rendering/TextureCache.hpp"
#include "Camera.hpp"
#include "GameObject.hpp"
#include "Component.hpp"
#include "CameraViewFrustum.hpp"
#include "Core/Logging/Log.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/LightDescriptor.hpp"
#include "Renderer/GlobalDescriptor.hpp"
#include "Renderer/RenderCmd.hpp"
#include "Renderer/CmdList.hpp"

#include "Physics/Physics.hpp"
#include "Filesystem/Filesystem.hpp"
#include "Animation/Animation.hpp"
#include "Audio/Audio.hpp"
#include "UI/UI.hpp"



namespace Recluse {


// Scene graph to push into the engine.
class Scene;

typedef void (*ControlInputCallback)();

// Engine object.
class Engine {
public:
  Engine();
  ~Engine();

  // Start up the engine with an initial window size. As the window is initialized 
  // along with the start up, be sure to manually call the Show() function from the 
  // window in order to see something!
  void                          StartUp(std::string appName, b8 fullscreen, i32 width = 800, i32 height = 600);
  void                          CleanUp();
  void                          SetControlInput(ControlInputCallback callback) { m_pControlInputFunc = callback; }
  void                          ProcessInput() { Window::PollEvents(); if (m_pControlInputFunc) m_pControlInputFunc(); }

  void                          Run();
  void                          SignalStop() { m_Stopping = true; }
  Camera*                       GetCamera() { return m_pCamera; }
  Window*                       GetWindow() { return &m_Window; }

  void                          Update();
  void                          SetCamera(Camera* camera) { m_pCamera = camera; m_CamFrustum.SetCamera(m_pCamera); }

  // Push the new scene to into this engine for extraction.
  void                          PushScene(Scene* scene) { m_pPushedScene = scene; }
  void                          BuildScene();
  void                          LoadSceneTransition();

  LightBuffer*                  LightData() { return m_pLights->Data(); }
  LightDescriptor*              LightDesc() { return m_pLights; }

  CmdList&                      RenderCommandList() { return m_RenderCmdList; }
  r64                           GameMousePosX() const { return m_GameMouseX; }
  r64                           GameMousePosY() const { return m_GameMouseY; }
  void                          SetGameMouseX(r64 x) { m_GameMouseX = x; }
  void                          SetGameMouseY(r64 y) { m_GameMouseY = y; }

  // TODO(): When new scene changes, we need to rebuild our commandbuffers in the 
  // renderer. This will need to be done by swapping old light material with new and 
  // rebuilding...
  b8                            Running() { return m_Running; }

private:

  void                          Stop();
  void                          UpdateRenderObjects();

  CCamViewFrustum               m_CamFrustum;
  LightDescriptor*              m_pLights;
  Camera*                       m_pCamera;
  Scene*                        m_pPushedScene;
  ControlInputCallback          m_pControlInputFunc;
  r64                           m_GameMouseX;
  r64                           m_GameMouseY;
  r64                           m_TimeAccumulate;

  Window                        m_Window;
  CmdList                       m_RenderCmdList;
  CmdList                       m_DeferredCmdList;
  b8                            m_Running : 1;
  b8                            m_Stopping : 1;
};


// Global engine.
Engine&     gEngine();
} // Recluse