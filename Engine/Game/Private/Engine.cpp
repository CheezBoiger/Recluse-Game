// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Engine.hpp"
#include "RendererComponent.hpp"
#include "MaterialComponent.hpp"
#include "LightComponent.hpp"
#include "PhysicsComponent.hpp"
#include "AudioComponent.hpp"

#include "Scene/Scene.hpp"
#include "Core/Thread/CoreThread.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Renderer/RenderObject.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/UserParams.hpp"

#include "GameObjectManager.hpp"

#include <queue>

namespace Recluse {


void KeyCallback(Window* window, i32 key, i32 scanCode, i32 action, i32 mods)
{
  Keyboard::keys[key] = (KeyAction)action;
  if (Keyboard::KeyPressed(KEY_CODE_2)) {
    Mouse::Show(!Mouse::Showing());
    Mouse::Enable(!Mouse::Enabled());
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
  if (action == Mouse::PRESSED) {
    Log() << "Button: " << ((button == Mouse::LEFT) ? "Left" : "Right");
    Log() << " pressed at location: X: " << Mouse::X() << " Y: " << Mouse::Y() << "\n"; 
  } else if (action == Mouse::RELEASED) {
    Log() << "Button: " << ((button == Mouse::LEFT) ? "Left" : "Right");
    Log() << " released at location: X: " << Mouse::X() << " Y: " << Mouse::Y() << "\n";
  }
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
  , m_dLag(0.0)
{
}


Engine::~Engine()
{
}


void Engine::StartUp(std::string appName, b8 fullscreen, i32 width, i32 height, const GraphicsConfigParams* params)
{
  if (m_running) return;

  // NOTE(): Always start up the core first, before starting anything else up.
  gCore().StartUp();
  gCore().ThrPool().RunAll();
  gFilesystem().StartUp();
  gRenderer().StartUp();
  gAnimation().StartUp();

#if !defined FORCE_PHYSICS_OFF
  gPhysics().StartUp();  
#endif
#if !defined FORCE_AUDIO_OFF
  gAudio().StartUp();
#endif

  gUI().StartUp();

  Window::SetKeyboardCallback(KeyCallback);
  Window::SetWindowResizeCallback(WindowResized);
  Window::SetMousePositionCallback(MousePositionMove);
  Window::SetMouseButtonCallback(MouseButtonClick);

  m_window.Create(appName, width, height);
  gRenderer().Initialize(&m_window, params);

  Material::InitializeDefault();
  LightComponent::GlobalInitialize();

  if (fullscreen) {
    m_window.SetToFullScreen();
  } else {
    m_window.SetToCenter();
  }
    

  m_cachedGameObjects.resize(1024);
  m_cachedGameObjectKeys.resize(1024);
}


void Engine::CleanUp()
{
  if (m_running) return;
  gCore().ThrPool().StopAll();
  if (!m_window.ShouldClose()) {
    m_window.Close();
    Window::PollEvents();
  }

  LightComponent::GlobalCleanUp();
  Material::CleanUpDefault();

  gUI().ShutDown();
#if !defined FORCE_AUDIO_OFF
  gAudio().ShutDown();
#endif
#if !defined FORCE_PHYSICS_OFF
  gPhysics().ShutDown();
#endif
  gAnimation().ShutDown();
  gRenderer().ShutDown();
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
  m_running = true;
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
  if (m_window.ShouldClose() || m_stopping) {
    Stop();
    return;
  }
  // Render out the scene.
  r64 dt = Time::DeltaTime * Time::ScaleTime;
  m_dLag += Time::DeltaTime;

  while (m_dLag >= Time::FixTime) {
    UpdateGameLogic();
    gAnimation().UpdateState(dt);
    gUI().UpdateState(dt);

#if !defined FORCE_AUDIO_OFF
    gAudio().UpdateState(dt);
#endif
#if !defined FORCE_PHYSICS_OFF
    gPhysics().UpdateState(dt);
    PhysicsComponent::UpdateComponents();
#endif
    m_dLag -= Time::FixTime;
  }

  gRenderer().Render();
}


void Engine::UpdateGameLogic()
{
  if (!m_pPushedScene) return;

  for ( u32 i = 0; i < m_sceneObjectCount; ++i ) {
    GameObject* object = m_cachedGameObjects[i];
    if ( object ) {
      object->Update(static_cast<r32>(Time::FixTime * Time::ScaleTime));
    }
  }

  PhysicsComponent::UpdateFromPreviousGameLogic();
  Transform::UpdateComponents();

  if (Camera::GetMain()) Camera::GetMain()->Update();

  std::thread worker0 = std::thread([&] () -> void {
    RendererComponent::UpdateComponents();
  });
  std::thread worker1 = std::thread([&] () -> void {
    PointLightComponent::UpdateComponents();
  });

  {
    DirectionalLight* pPrimary = m_pPushedScene->GetPrimaryLight();
    LightBuffer* pLights = gRenderer().LightData();
    pLights->_PrimaryLight._Ambient = pPrimary->_Ambient;
    pLights->_PrimaryLight._Color = pPrimary->_Color;
    pLights->_PrimaryLight._Direction = pPrimary->_Direction;
    pLights->_PrimaryLight._Enable = pPrimary->_Enable;
    pLights->_PrimaryLight._Intensity = pPrimary->_Intensity;
  }

  

  worker0.join();
  worker1.join();
 //TraverseScene(UpdateGameObject);
}


void BuildSceneCallback(Engine* engine, GameObject* object, size_t currNum)
{ 
  auto&     cache = engine->GetGameObjectCache();
  auto&     cachedKeys = engine->GetGameObjectKeys();

  if (currNum >= cache.size()) {
    cache.resize(cache.size() << 1);
    cachedKeys.resize(cachedKeys.size() << 1);
  }

  cache[currNum] = object;
  cachedKeys[currNum] = object->GetId();
  
  
}


void Engine::BuildScene()
{
  if (!m_pPushedScene) return;
  m_cachedGameObjects.clear();
  m_cachedGameObjectKeys.clear();
  m_cachedGameObjectKeys.resize(1);
  m_cachedGameObjects.resize(1);

  TraverseScene(BuildSceneCallback);
  gRenderer().Build();
  gRenderer().PushRenderIds(m_cachedGameObjectKeys.data(), 
    static_cast<u32>(m_cachedGameObjectKeys.size()));
}


void Engine::TraverseScene(GameObjectActionCallback callback)
{
  // Traversing the scene graph using DFS.
  // TODO(): Probably want to make a real stack allocator that doesn't have a terrible
  // amortized time complex like this vector.
  std::vector<GameObject*> nodes;
  m_sceneObjectCount = 0;
  SceneNode* root = m_pPushedScene->GetRoot();

  for (size_t i = 0; i < root->GetChildCount(); ++i) {
    nodes.push_back(root->GetChild(i));
  }
  
  while (!nodes.empty()) {
    GameObject* object = nodes.back();
    nodes.pop_back();

    callback(this, object, m_sceneObjectCount);
    m_sceneObjectCount++;

    // Now query its children.
    size_t child_count = object->GetChildrenCount();
    for (size_t i = 0; i < child_count; ++i) {
      GameObject* child = object->GetChild(i);
      nodes.push_back(child);
    }
  }
}
} // Recluse