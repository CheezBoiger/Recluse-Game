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
  object->getTransform()->update();
}


void KeyCallback(Window* window, i32 key, i32 scanCode, i32 action, i32 mods)
{
  Keyboard::keys[key] = (KeyAction)action;
  if (Keyboard::KeyPressed(KEY_CODE_2)) {
    Mouse::show(!Mouse::isShowing());
    Mouse::setEnable(!Mouse::isEnabled());
    Mouse::setTrack(!Mouse::isTracking());
    if (Mouse::isEnabled()) {
      Mouse::setPosition( window->getWidth() * 0.5 + window->getX(), 
                          window->getHeight() * 0.5 + window->getY()
      );
    } else {
      Mouse::setPosition( gEngine().GameMousePosX(), 
                          gEngine().GameMousePosY());
    }
  }
}


void WindowResized(Window* window, i32 width, i32 height)
{
  if (gRenderer().isActive() && gRenderer().isInitialized()) {
    gRenderer().updateRendererConfigs(nullptr);
    R_DEBUG(rVerbose, "Engine Renderer has been updated prior to resize.\n");
  }
  
}


void MousePositionMove(Window* window, r64 x, r64 y)
{
  Camera* camera = Camera::getMain();
  if (camera && !Mouse::isEnabled()) {
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


void Engine::startUp(std::string appName, b32 fullscreen, i32 width, i32 height, const GraphicsConfigParams* params)
{
  if (m_running) return;

  // NOTE(): Always start up the core first, before starting anything else up.
  gCore().startUp();
  gCore().ThrPool().RunAll();
  gFilesystem().startUp();

  Window::setKeyboardCallback(KeyCallback);
  Window::setWindowResizeCallback(WindowResized);
  Window::setMousePositionCallback(MousePositionMove);
  Window::setMouseButtonCallback(MouseButtonClick);

  m_window.create(appName, width, height);

  if (fullscreen) {
    m_window.setToFullScreen();
  }
  else {
    m_window.setToCenter();
  }

  // For renderer, you want to send the name to the device that will be used for debugging and
  // information for vendors.
  gRenderer().setAppName(appName.c_str());
  //gRenderer().setHardwareHints(R_RAYTRACING_BIT | R_MESHSHADER_BIT);
  gRenderer().startUp();
  gRenderer().initialize(&m_window, params);

  Material::initializeDefault(&gRenderer());
  LightComponent::globalInitialize();


  gAnimation().startUp();
#if !defined FORCE_PHYSICS_OFF
  gPhysics().startUp();
#endif
#if !defined FORCE_AUDIO_OFF
  gAudio().startUp();
#endif
  gUI().startUp();
}


void Engine::cleanUp()
{
  if (m_running) return;
  gCore().ThrPool().StopAll();

  LightComponent::globalCleanUp();
  Material::cleanUpDefault(&gRenderer());

  gUI().shutDown();
#if !defined FORCE_AUDIO_OFF
  gAudio().shutDown();
#endif
#if !defined FORCE_PHYSICS_OFF
  gPhysics().shutDown();
#endif
  gAnimation().shutDown();
  gRenderer().shutDown();

  if (!m_window.shouldClose()) {
    m_window.close();
    Window::pollEvents();
  }

  gFilesystem().shutDown();
  gCore().shutDown();
  m_running = false;
}


void Engine::run()
{
  if (m_running) return;
  // TODO(): Signal to continue thread works.

  // Start up the time as the engine begins running.
  Time::start();
  gRenderer().build();
  m_running = true;

  // Update once.
  update();
}


void Engine::stop()
{
  if (!m_running) return;
  // TODO(): Signal to stop thread works.

  gRenderer().waitIdle();
  m_running = false;
}


void Engine::update()
{
  // TODO(): Work on loop update step a little more...

  if (m_window.shouldClose() || m_stopping) {
    stop();
    return;
  }
  // render out the scene.
  r64 dt = Time::deltaTime;
  r64 tick = Time::fixTime;
  m_dLag += Time::deltaTime;


#if !defined FORCE_AUDIO_OFF
  gAudio().updateState(dt);
#endif

  // Update using next frame input.
  AnimationComponent::updateComponents();
  gAnimation().updateState(dt);
  
  PhysicsComponent::UpdateFromPreviousGameLogic();
  traverseScene(UpdateTransform);
  updateSunLight();

  m_workers[0] = std::thread([&] () -> void {
    gPhysics().updateState(dt, tick);
    PhysicsComponent::updateComponents();
  });

  m_workers[1] = std::thread([&]() -> void {
    PointLightComponent::updateComponents();
    SpotLightComponent::updateComponents();
  });

  m_workers[2] = std::thread([&]() -> void {
    MeshComponent::updateComponents();
    AbstractRendererComponent::updateComponents();
    //SkinnedRendererComponent::updateComponents();
  });

  ParticleSystemComponent::updateComponents();
  gUI().updateState(dt);

  m_workers[0].join();
  m_workers[1].join();
  m_workers[2].join();

  {
    Camera* pMain = Camera::getMain();
    if (pMain) {
      pMain->flushToGpuBus();
    }
  }

  switch (m_engineMode) {
    case EngineMode_Bake:
    {
      static const char* probname = "Probe";
      for (size_t i = 0; i < m_envProbeTargets.size(); ++i) {
        Vector3 position = m_envProbeTargets[i];
        TextureCube* cube = gRenderer().bakeEnvironmentMap(position);
        std::string name = std::string(probname) + std::to_string(i) + ".png";
        Log(rNotify) << "probe : " << name << " baking.";
        cube->Save(name.c_str());
        Log() << " Done!\n";
        gRenderer().freeTextureCube(cube);
      }
      break;
    }
    case EngineMode_Game:
    default:
    {
      gRenderer().render();
      break;
    }
  }
}


void Engine::updateSunLight()
{
  if (!m_pPushedScene) return;
  Sky* pSky = m_pPushedScene->getSky();
  DirectionalLight* pPrimary = pSky->getSunLight();
  LightBuffer* pLights = gRenderer().getLightData();
  pLights->_PrimaryLight._Ambient = pPrimary->_Ambient;
  pLights->_PrimaryLight._Color = pPrimary->_Color;
  pLights->_PrimaryLight._Direction = pPrimary->_Direction;
  pLights->_PrimaryLight._Enable = pPrimary->_Enable;
  pLights->_PrimaryLight._Intensity = pPrimary->_Intensity;
}


void Engine::traverseScene(GameObjectActionCallback callback)
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
  SceneNode* root = m_pPushedScene->getRoot();
  for (size_t i = 0; i < root->getChildrenCount(); ++i) {
    nodes[++top] = root->getChild(i);
    if (top >= (i32(nodes.size()) - 1)) { nodes.resize(nodes.size() << 1); }
  }
  
  while (top != -1) {
    GameObject* object = nodes[top--];

    callback(this, object, m_sceneObjectCount);
    m_sceneObjectCount++;

    // Now query its children.
    size_t child_count = object->getChildrenCount();
    for (size_t i = 0; i < child_count; ++i) {
      GameObject* child = object->getChild(i);
      nodes[++top] = child;
      if (top >= (i32(nodes.size()) - 1)) { nodes.resize(nodes.size() << 1); }
    }
  }
}


void Engine::pushScene(Scene* scene)
{
  R_ASSERT(scene, "Attempting to push a null scene!");
  m_pPushedScene = scene;
  gRenderer().adjustHDRSettings(scene->getHDRSettings());
}
} // Recluse