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
#include "LightComponent.hpp"
#include "PointLightComponent.hpp"
#include "MaterialComponent.hpp"
#include "MeshComponent.hpp"
#include "RendererComponent.hpp"
#include "PointLightComponent.hpp"
#include "CameraViewFrustum.hpp"
#include "Core/Logging/Log.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/LightDescriptor.hpp"
#include "Renderer/GlobalDescriptor.hpp"
#include "Renderer/RenderCmd.hpp"
#include "Renderer/CmdList.hpp"
#include "Renderer/UserParams.hpp"

#include "Physics/Physics.hpp"
#include "Audio/Audio.hpp"
#include "Filesystem/Filesystem.hpp"
#include "Animation/Animation.hpp"
#include "Audio/Audio.hpp"
#include "UI/UI.hpp"


namespace Recluse {


// Scene graph to push into the engine.
class Scene;

typedef void (*ControlInputCallback)();


struct InputAxis {
  KeyAction       _Pos;
  KeyAction       _Neg;
  r32             _Weight;
  // dt scale movement.
  r32             _Scale;
};


// Engine object.
class Engine {
public:
  // Callback to determine the manipulation of a game object in this engine.
  typedef void (*GameObjectActionCallback)(Engine*, GameObject*, size_t);
  
  Engine();
  ~Engine();

  // Start up the engine with an initial window size. As the window is initialized 
  // along with the start up, be sure to manually call the Show() function from the 
  // window in order to see something!
  // GpuConfigParams is an optional setting for initial start up of the renderer, you may pass nullptr for default settings.
  void                          StartUp(std::string appName, b8 fullscreen, i32 width = 800, i32 height = 600, const GraphicsConfigParams* params = nullptr);
  void                          CleanUp();
  
  // TODO(): Engine will no longer need a Control Input, instead, it will have InputAxises.
  void                          SetControlInput(ControlInputCallback callback) { m_pControlInputFunc = callback; }
  void                          ProcessInput() { Window::PollEvents(); if (m_pControlInputFunc) m_pControlInputFunc(); }

  void                          Run();
  void                          SignalStop() { m_stopping = true; }
  Camera*                       GetCamera() { return m_pCamera; }
  Window*                       GetWindow() { return &m_window; }

  void                          Update();
  void                          SetCamera(Camera* camera) { m_pCamera = camera; m_camFrustum.SetCamera(m_pCamera); }

  // Push the new scene to into this engine for extraction.
  void                          PushScene(Scene* scene) { m_pPushedScene = scene; }
  void                          BuildScene();
  void                          LoadSceneTransition();
  std::vector<GameObject*>&     GetGameObjectCache() { return m_cachedGameObjects; }
  std::vector<game_uuid_t>&     GetGameObjectKeys() { return m_cachedGameObjectKeys; } 

  r64                           GameMousePosX() const { return m_gameMouseX; }
  r64                           GameMousePosY() const { return m_gameMouseY; }
  void                          SetGameMouseX(r64 x) { m_gameMouseX = x; }
  void                          SetGameMouseY(r64 y) { m_gameMouseY = y; }

  u32                           GetSceneObjectCount() const { return m_sceneObjectCount; }
  // TODO(): When new scene changes, we need to rebuild our commandbuffers in the 
  // renderer. This will need to be done by swapping old light material with new and 
  // rebuilding...
  b8                            Running() { return m_running; }

private:

  void                          Stop();
  void                          UpdateGameLogic();
  void                          TraverseScene(GameObjectActionCallback callback);

  std::vector<GameObject*>      m_cachedGameObjects;
  std::vector<game_uuid_t>      m_cachedGameObjectKeys;
  CCamViewFrustum               m_camFrustum;
  Camera*                       m_pCamera;
  Scene*                        m_pPushedScene;
  ControlInputCallback          m_pControlInputFunc;
  r64                           m_gameMouseX;
  r64                           m_gameMouseY;
  r64                           m_dLag;

  Window                        m_window;
  u32                           m_sceneObjectCount;
  b8                            m_running : 1;
  b8                            m_stopping : 1;
};


// Global engine.
Engine&     gEngine();
} // Recluse