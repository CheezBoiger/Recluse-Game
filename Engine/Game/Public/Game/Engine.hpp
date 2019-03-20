// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Core.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Win32/Keyboard.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/ViewFrustum.hpp"

#include "Geometry/Cube.hpp"
#include "Rendering/TextureCache.hpp"
#include "Scene/AssetManager.hpp"
#include "Camera.hpp"
#include "GameObject.hpp"
#include "Component.hpp"
#include "LightComponent.hpp"
#include "PointLightComponent.hpp"
#include "AnimationComponent.hpp"
#include "PhysicsComponent.hpp"
#include "MeshComponent.hpp"
#include "RendererComponent.hpp"
#include "PointLightComponent.hpp"
#include "Rendering/RendererResourcesCache.hpp"
#include "Core/Logging/Log.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/LightDescriptor.hpp"
#include "Renderer/GlobalDescriptor.hpp"
#include "Renderer/RenderCmd.hpp"
#include "Renderer/CmdList.hpp"
#include "Renderer/UserParams.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Mesh.hpp"

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


enum EngineMode {
  EngineMode_Game,
  EngineMode_Bake
};


// Engine object.
class Engine {
  static const size_t kMaxViewFrustums = 32;
public:
  // Callback to determine the manipulation of a game object in this engine.
  typedef void (*GameObjectActionCallback)(Engine*, GameObject*, size_t);
  
  Engine();
  ~Engine();

  // Start up the engine with an initial window size. As the window is initialized 
  // along with the start up, be sure to manually call the show() function from the 
  // window in order to see something!
  // GpuConfigParams is an optional setting for initial start up of the renderer, you may pass nullptr for default settings.
  void                          startUp(std::string appName, b32 fullscreen, i32 width = 800, i32 height = 600, const GraphicsConfigParams* params = nullptr);
  void                          cleanUp();
  
  // TODO(): Engine will no longer need a Control Input, instead, it will have InputAxises.
  void                          setControlInput(ControlInputCallback callback) { m_pControlInputFunc = callback; }
  void                          processInput() { Window::pollEvents(); if (m_pControlInputFunc) m_pControlInputFunc(); }

  void                          run();
  void                          signalStop() { m_stopping = true; }
  Window*                       getWindow() { return &m_window; }

  void                          update();

  // Push the new scene to into this engine for extraction.
  void                          pushScene(Scene* scene);

  // Transitions from one scene to another if needed. This is optional.
  void                          signalLoadSceneTransition() { m_bSignalLoadScene = true; }

  Scene*                        getScene() { return m_pPushedScene; } 

  r64                           GameMousePosX() const { return m_gameMouseX; }
  r64                           GameMousePosY() const { return m_gameMouseY; }
  void                          SetGameMouseX(r64 x) { m_gameMouseX = x; }
  void                          SetGameMouseY(r64 y) { m_gameMouseY = y; }

  void                          EnableMultithreading(b32 enable) { m_multiThreading = enable; }
  u32                           GetSceneObjectCount() const { return m_sceneObjectCount; }
  // TODO(): When new scene changes, we need to rebuild our commandbuffers in the 
  // renderer. This will need to be done by swapping old light material with new and 
  // rebuilding...
  b32                           isRunning() { return m_running; }
  b32                           isMultiThreading() const { return m_multiThreading; }

  // Get array of view frustum references.
  ViewFrustum**                 getViewFrustums() { return m_frustums; }  

  // Add a frustum to the engine for culling, returns the index of which the frustum was stored in the 
  // engine's reference array.
  i32 addFrustum(ViewFrustum* frustum) { 
    i32 c = m_currFrustumCount; 
    m_frustums[m_currFrustumCount++] = frustum; 
    return c; 
  } 

  size_t                        getViewFrustumCount() const { return m_currFrustumCount; }
  
  static size_t                 getMaxViewFrustumCount() { return kMaxViewFrustums; }

  void                          setEngineMode(EngineMode newMode) { m_engineMode = newMode; }

  EngineMode                    getEngineMode() const { return m_engineMode; }

  // Set Probe target positions to begin baking texture cubemaps, which will be stored into the 
  // set scene.
  void                          setEnvProbeTargets(Vector3* positions, u32 count) {
    m_envProbeTargets.resize(count);
    for (size_t i = 0; i < count; ++i) {
      m_envProbeTargets[i] = positions[i];
    }
  };

  void                          clearProbeTargets() { m_envProbeTargets.clear(); }

private:

  void                          stop();
  void                          traverseScene(GameObjectActionCallback callback);
  void                          updateSunLight();

  Scene*                        m_pPushedScene;
  ControlInputCallback          m_pControlInputFunc;
  r64                           m_gameMouseX;
  r64                           m_gameMouseY;
  r64                           m_dLag;

  Window                        m_window;
  u32                           m_sceneObjectCount;
  b32                           m_running  : 1;
  b32                           m_stopping : 1;
  b32                           m_multiThreading : 1;
  b32                           m_bSignalLoadScene;
  std::vector<std::thread>      m_workers;
  ViewFrustum*                  m_frustums[kMaxViewFrustums];
  i32                           m_currFrustumCount;
  EngineMode                    m_engineMode;
  std::vector<Vector3>          m_envProbeTargets;
};


// Global engine.
Engine&     gEngine();
} // Recluse