// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Engine.hpp"
#include "RendererComponent.hpp"
#include "MaterialComponent.hpp"
#include "LightComponent.hpp"
#include "PhysicsComponent.hpp"
#include "ParticleSystemComponent.hpp"
#include "AudioComponent.hpp"

#include "Scene/Scene.hpp"
#include "Core/Thread/CoreThread.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/UserParams.hpp"

#include "GameObjectManager.hpp"

#include <queue>

namespace Recluse {


void UpdateTransform(Engine* engine, GameObject* object, size_t currNum)
{ 
  object->GetTransform()->Update();
}


void KeyCallback(Window* window, i32 key, i32 scanCode, i32 action, i32 mods)
{
  Keyboard::keys[key] = (KeyAction)action;
  if (Keyboard::KeyPressed(KEY_CODE_2)) {
    Mouse::Show(!Mouse::Showing());
    Mouse::Enable(!Mouse::Enabled());
    Mouse::Track(!Mouse::Tracking());
    if (Mouse::Enabled()) {
      Mouse::SetPosition( window->Width() * 0.5 + window->X(), 
                          window->Height() * 0.5 + window->Y()
      );
    } else {
      Mouse::SetPosition( gEngine().GameMousePosX(), 
                          gEngine().GameMousePosY());
    }
  }
}


void WindowResized(Window* window, i32 width, i32 height)
{
  if (gRenderer().IsActive() && gRenderer().Initialized()) {
    gRenderer().UpdateRendererConfigs(nullptr);
    R_DEBUG(rVerbose, "Engine Renderer has been updated prior to resize.\n");
  }
  
}


void MousePositionMove(Window* window, r64 x, r64 y)
{
  Camera* camera = Camera::GetMain();
  if (camera && !Mouse::Enabled()) {
    gEngine().SetGameMouseX(x);
    gEngine().SetGameMouseY(y);
  }
}


void MouseButtonClick(Window* window, i32 button, i32 action, i32 mod)
{
#if 0
  if (action == Mouse::PRESSED) {
    Log() << "Button: " << ((button == Mouse::LEFT) ? "Left" : "Right");
    Log() << " pressed at location: X: " << Mouse::X() << " Y: " << Mouse::Y() << "\n"; 
  } else if (action == Mouse::RELEASED) {
    Log() << "Button: " << ((button == Mouse::LEFT) ? "Left" : "Right");
    Log() << " released at location: X: " << Mouse::X() << " Y: " << Mouse::Y() << "\n";
  }
#endif
}


Engine& gEngine()
{
  static Engine engine;
  return engine;
}


Engine::Engine()
  : m_pPushedScene(nullptr)
  , m_gameMouseX(0.0)
  , m_gameMouseY(0.0)
  , m_sceneObjectCount(0)
  , m_pControlInputFunc(nullptr)
  , m_running(false)
  , m_stopping(false)
  , m_bSignalLoadScene(false)
  , m_dLag(0.0)
  , m_engineMode(EngineMode_Game)
{
  m_workers.resize(4);
  for (size_t i = 0; i < kMaxViewFrustums; ++i) {
    m_frustums[i] = nullptr;
  }
}


Engine::~Engine()
{
}


void Engine::StartUp(std::string appName, b32 fullscreen, i32 width, i32 height, const GraphicsConfigParams* params)
{
  if (m_running) return;

  // NOTE(): Always start up the core first, before starting anything else up.
  gCore().StartUp();
  gCore().ThrPool().RunAll();
  gFilesystem().StartUp();

  Window::SetKeyboardCallback(KeyCallback);
  Window::SetWindowResizeCallback(WindowResized);
  Window::SetMousePositionCallback(MousePositionMove);
  Window::SetMouseButtonCallback(MouseButtonClick);

  m_window.Create(appName, width, height);

  if (fullscreen) {
    m_window.SetToFullScreen();
  }
  else {
    m_window.SetToCenter();
  }

  // For renderer, you want to send the name to the device that will be used for debugging and
  // information for vendors.
  gRenderer().SetAppName(appName.c_str());
  gRenderer().StartUp();
  gRenderer().Initialize(&m_window, params);

  Material::InitializeDefault(&gRenderer());
  LightComponent::GlobalInitialize();


  gAnimation().StartUp();
#if !defined FORCE_PHYSICS_OFF
  gPhysics().StartUp();
#endif
#if !defined FORCE_AUDIO_OFF
  gAudio().StartUp();
#endif
  gUI().StartUp();
}


void Engine::CleanUp()
{
  if (m_running) return;
  gCore().ThrPool().StopAll();

  LightComponent::GlobalCleanUp();
  Material::CleanUpDefault(&gRenderer());

  gUI().ShutDown();
#if !defined FORCE_AUDIO_OFF
  gAudio().ShutDown();
#endif
#if !defined FORCE_PHYSICS_OFF
  gPhysics().ShutDown();
#endif
  gAnimation().ShutDown();
  gRenderer().ShutDown();

  if (!m_window.ShouldClose()) {
    m_window.Close();
    Window::PollEvents();
  }

  gFilesystem().ShutDown();
  gCore().ShutDown();
  m_running = false;
}


void Engine::Run()
{
  if (m_running) return;
  // TODO(): Signal to continue thread works.

  // Start up the time as the engine begins running.
  Time::Start();
  gRenderer().Build();
  m_running = true;

  // Update once.
  Update();
}


void Engine::Stop()
{
  if (!m_running) return;
  // TODO(): Signal to stop thread works.

  gRenderer().WaitIdle();
  m_running = false;
}


void Engine::Update()
{
  // TODO(): Work on loop update step a little more...

  if (m_window.ShouldClose() || m_stopping) {
    Stop();
    return;
  }
  // Render out the scene.
  r64 dt = Time::DeltaTime;
  r64 tick = Time::FixTime;
  m_dLag += Time::DeltaTime;


#if !defined FORCE_AUDIO_OFF
  gAudio().UpdateState(dt);
#endif

  // Update using next frame input.
  AnimationComponent::UpdateComponents();
  gAnimation().UpdateState(dt);
  
  PhysicsComponent::UpdateFromPreviousGameLogic();
  TraverseScene(UpdateTransform);
  UpdateSunLight();

  m_workers[0] = std::thread([&] () -> void {
    gPhysics().UpdateState(dt, tick);
    PhysicsComponent::UpdateComponents();
  });

  m_workers[1] = std::thread([&]() -> void {
    PointLightComponent::UpdateComponents();
    SpotLightComponent::UpdateComponents();
  });

  m_workers[2] = std::thread([&]() -> void {
    MeshComponent::UpdateComponents();
    RendererComponent::UpdateComponents();
    SkinnedRendererComponent::UpdateComponents();
  });

  ParticleSystemComponent::UpdateComponents();
  gUI().UpdateState(dt);

  m_workers[0].join();
  m_workers[1].join();
  m_workers[2].join();

  {
    Camera* pMain = Camera::GetMain();
    if (pMain) {
      pMain->FlushToGpuBus();
    }
  }

  switch (m_engineMode) {
    case EngineMode_Bake:
    {
      static const char* probname = "Probe";
      for (size_t i = 0; i < m_envProbeTargets.size(); ++i) {
        Vector3 position = m_envProbeTargets[i];
        TextureCube* cube = gRenderer().BakeEnvironmentMap(position);
        std::string name = std::string(probname) + std::to_string(i) + ".png";
        Log(rNotify) << "probe : " << name << " baking.";
        cube->Save(name.c_str());
        Log() << " Done!\n";
        gRenderer().FreeTextureCube(cube);
      }
      break;
    }
    case EngineMode_Game:
    default:
    {
      gRenderer().Render();
      break;
    }
  }
}


void Engine::UpdateSunLight()
{
  if (!m_pPushedScene) return;
  Sky* pSky = m_pPushedScene->GetSky();
  DirectionalLight* pPrimary = pSky->GetSunLight();
  LightBuffer* pLights = gRenderer().LightData();
  pLights->_PrimaryLight._Ambient = pPrimary->_Ambient;
  pLights->_PrimaryLight._Color = pPrimary->_Color;
  pLights->_PrimaryLight._Direction = pPrimary->_Direction;
  pLights->_PrimaryLight._Enable = pPrimary->_Enable;
  pLights->_PrimaryLight._Intensity = pPrimary->_Intensity;
}


void Engine::TraverseScene(GameObjectActionCallback callback)
{
  if (!m_pPushedScene) {
    R_DEBUG(rWarning, "No scene to run, skipping scene graph traversal.");
    return;
  }
  // Traversing the scene graph using DFS.
  // TODO(Mario): This is too primitive (what if there are more than this many game objects
  // in the scene?) Need a stack that can be resized.
  static std::vector<GameObject*> nodes(1024);
  i32 top = -1;
  m_sceneObjectCount = 0;
  SceneNode* root = m_pPushedScene->GetRoot();
  for (size_t i = 0; i < root->GetChildrenCount(); ++i) {
    nodes[++top] = root->GetChild(i);
    if (top >= (i32(nodes.size()) - 1)) { nodes.resize(nodes.size() << 1); }
  }
  
  while (top != -1) {
    GameObject* object = nodes[top--];

    callback(this, object, m_sceneObjectCount);
    m_sceneObjectCount++;

    // Now query its children.
    size_t child_count = object->GetChildrenCount();
    for (size_t i = 0; i < child_count; ++i) {
      GameObject* child = object->GetChild(i);
      nodes[++top] = child;
      if (top >= (i32(nodes.size()) - 1)) { nodes.resize(nodes.size() << 1); }
    }
  }
}


void Engine::PushScene(Scene* scene)
{
  R_ASSERT(scene, "Attempting to push a null scene!");
  m_pPushedScene = scene;
  gRenderer().AdjustHDRSettings(scene->GetHDRSettings());
}
} // Recluse