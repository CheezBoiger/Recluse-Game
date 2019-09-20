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
#include <cctype>
#include <algorithm>

namespace Recluse {


void UpdateTransform(Engine* engine, GameObject* object, size_t currNum)
{ 
  object->getTransform()->update();
}


void KeyCallback(Window* window, I32 key, I32 scanCode, I32 action, I32 mods)
{
  switch ( ( KeyAction )action )
  {
    case KEY_DOWN:
    {
      if (Keyboard::keys[key] == KEY_DOWN)
      {
        Keyboard::keys[key] = KEY_STILLDOWN;
      }
      else if (Keyboard::keys[key] != KEY_STILLDOWN)
      { 
        Keyboard::keys[key] = KEY_DOWN;
      }
    } break;
    case KEY_UP:
    {
      Keyboard::keys[ key ] = KEY_UP; 
    } break;
    default:
      Keyboard::keys[key] = (KeyAction)action; break;
  }
#if 0
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
#endif
}


void WindowResized(Window* window, I32 width, I32 height)
{
  if ( gRenderer( ).isActive( ) && gRenderer( ).isInitialized( ) ) 
  {
    Mouse::resetButtonActions( );
    gRenderer( ).updateRendererConfigs( nullptr );
    R_DEBUG( rVerbose, "Engine Renderer has been updated prior to resize.\n" );
  }
  
}


void MouseButtonClick(Window* window, I32 button, I32 action, I32 mod)
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
  , m_physicsAccum(0.0)
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


void Engine::processInput() 
{
  Window::pollEvents();
  if (m_pControlInputFunc) m_pControlInputFunc();
}



void Engine::startUp(std::string appName, const UserConfigParams* userParams, const GraphicsConfigParams* params)
{
  if (m_running) return;

  // NOTE(): Always start up the core first, before starting anything else up.
  gCore().startUp();
  gCore().ThrPool().RunAll();
  gFilesystem().startUp();

  Window::setKeyboardCallback(KeyCallback);
  Window::setWindowResizeCallback(WindowResized);
  Window::setMouseButtonCallback(MouseButtonClick);

  m_window.create(appName, userParams->_windowWidth, userParams->_windowHeight);

  if (userParams->_windowType == WindowType_Fullscreen) {
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
  // TODO(): getSignal to continue thread works.

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
  // TODO(): getSignal to stop thread works.

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
  R64 dt = Time::deltaTime;
  R64 tick = Time::fixTime;
  //m_physicsAccum += Time::deltaTime;


  // Update using next frame input.
  gUI().updateState(dt);

  AnimationComponent::updateComponents();
  gAnimation().updateState(dt);
  
  traverseScene(UpdateTransform);
  updateSunLight();

  m_workers[0] = std::thread([&] () -> void {
    //while (m_physicsAccum >= tick) {
    PhysicsComponent::UpdateFromPreviousGameLogic();
    gPhysics().updateState(dt, tick);
    PhysicsComponent::updateComponents();
      //m_physicsAccum -= tick;
    //}

#if !defined FORCE_AUDIO_OFF
    AudioComponent::updateComponents();
    gAudio().updateState(dt);
#endif
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
        cube->save(name.c_str());
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
  I32 top = -1;
  m_sceneObjectCount = 0;
  SceneNode* root = m_pPushedScene->getRoot();
  for (size_t i = 0; i < root->getChildrenCount(); ++i) {
    nodes[++top] = root->getChild(i);
    if (top >= (I32(nodes.size()) - 1)) { nodes.resize(nodes.size() << 1); }
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
      if (top >= (I32(nodes.size()) - 1)) { nodes.resize(nodes.size() << 1); }
    }
  }
}


void Engine::pushScene(Scene* scene)
{
  R_ASSERT(scene, "Attempting to push a null scene!");
  m_pPushedScene = scene;
  gRenderer().adjustHDRSettings(scene->getHDRSettings());
}



std::string getOption(const std::string& line) 
{
  size_t pos = line.find('=');
  if (pos == std::string::npos) return "";
  std::string option = line.substr(pos + 1);
  option.erase(std::remove_if(option.begin(), option.end(),
                              [](U8 x) -> I32 { return std::isspace(x); }),
               option.end());
  std::transform(option.begin(), option.end(), option.begin(), std::tolower);
  return option;
}


B32 availableOption(const std::string& line, const TChar* option) 
{
  size_t pos = line.find(option);
  if (pos != std::string::npos && pos == 1) {
    size_t b = line.find("]");
    std::string key = line.substr(pos, b - pos);
    if (key.compare(option) == 0)
        return true;
    }
  return false;
}


void Engine::readGraphicsConfig( GraphicsConfigParams& graphics )
{
  FileHandle Buf;
  FilesystemResult result =
      gFilesystem().ReadFrom("Configs/RendererConfigs.recluse", &Buf);
  if (result == FilesystemResult_NotFound) {
    graphics = kDefaultGpuConfigs;
    Log(rWarning) << "RendererConfigs not found! Setting default rendering "
                     "configuration.\n";
    return;
  }

  std::string line = "";
  for (size_t i = 0; i < Buf.Sz; ++i) {
    TChar ch = Buf.Buf[i];
    line.push_back(ch);
    if (ch == '\n' || i == (Buf.Sz - 1)) {
      Log(rDebug) << line;
      if (availableOption(line, "Buffering")) {
        std::string option = getOption(line);
        if (option.compare("triple") == 0) {
          graphics._Buffering = TRIPLE_BUFFER;
        } else if (option.compare("single") == 0) {
          graphics._Buffering = SINGLE_BUFFER;
        } else {
          graphics._Buffering = DOUBLE_BUFFER;
        }
      }
      if (availableOption(line, "AntiAliasing")) {
        std::string option = getOption(line);
        if (option.compare("fxaa") == 0) {
          graphics._AA = AA_FXAA_2x;
        } else if (option.compare("smaa2x") == 0) {
          graphics._AA = AA_SMAA_2x;
        } else {
          graphics._AA = AA_None;
        }
      }
      if (availableOption(line, "TextureQuality")) {
        std::string option = getOption(line);
      }
      if (availableOption(line, "ShadowQuality")) {
        std::string option = getOption(line);
        if (option.compare("ultra") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_ULTRA;
        } else if (option.compare("high") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_HIGH;
        } else if (option.compare("medium") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_MEDIUM;
        } else if (option.compare("low") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_LOW;
        } else {
          graphics._Shadows = GRAPHICS_QUALITY_NONE;
        }
      }
      if (availableOption(line, "DesiredSwapImages")) {
        std::string option = getOption(line);
        U32 images = std::atoi(option.c_str());
        graphics._desiredSwapImages = images;
      }
      if (availableOption(line, "LightingQuality")) {
        std::string option = getOption(line);
      }
      if (availableOption(line, "ModelQuality")) {
        std::string option = getOption(line);
      }
      if (availableOption(line, "LevelOfDetail")) {
        std::string option = getOption(line);
      }
      if (availableOption(line, "RenderScale")) {
        std::string option = getOption(line);
      }
      if (availableOption(line, "VSync")) {
        std::string option = getOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableVsync = true;
        } else {
          graphics._EnableVsync = false;
        }
      }
      if (availableOption(line, "ChromaticAberration")) {
        std::string option = getOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableChromaticAberration = true;
        } else {
          graphics._EnableChromaticAberration = false;
        }
      }
      if (availableOption(line, "Bloom")) {
        std::string option = getOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableBloom = true;
        } else {
          graphics._EnableBloom = false;
        }
      }
      if (availableOption(line, "PostProcessing")) {
        std::string option = getOption(line);
        if (option.compare("true") == 0) {
          graphics._EnablePostProcessing = true;
        } else {
          graphics._EnablePostProcessing = false;
        }
      }
      if (availableOption(line, "SoftShadows")) {
        std::string option = getOption(line);
        if (option.compare("true") == 0) {
          graphics._enableSoftShadows = true;
        } else {
          graphics._enableSoftShadows = false;
        }
      }
      if (availableOption(line, "RenderMultithreading")) {
        std::string option = getOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableMultithreadedRendering = true;
        } else {
          graphics._EnableMultithreadedRendering = false;
        }
      }
      if ( availableOption( line, "CascadeShadowMapResolution" ) ) {
        std::string option = getOption( line );
        U32 res = atoi( option.c_str( ) );
        graphics._cascadeShadowMapRes = res;
      }
      if ( availableOption( line, "NumberCascadeShadowMaps" ) ) {
        std::string option = getOption( line );
        U32 num = atoi( option.c_str( ) );
        graphics._numberCascadeShadowMaps = num;
      }
      if (availableOption(line, "ShadowMapPointLightResolution")) {
        std::string option = getOption(line);
        U32 res = atoi(option.c_str());
        graphics._shadowMapArrayRes = res;
      }
      if (availableOption(line, "EnableFrameLimit")) {
        std::string option = getOption(line);
        if (option.compare("true") == 0) {
          graphics._enableFrameLimit = true;
        } else {
          graphics._enableFrameLimit = false;
        }
      }
      if (availableOption(line, "FrameLimit")) {
        std::string option = getOption(line);
        U32 frameLimit = std::atoi(option.c_str());
        graphics._frameLimit = frameLimit;
      }
      if (availableOption(line, "RenderResolutionWidth")) {
        std::string option = getOption(line);
        U32 rW = std::atoi(option.c_str());
        graphics._renderResolutionWidth = rW;
      }
      if (availableOption(line, "RenderResolutionHeight")) {
          std::string option = getOption(line);
          U32 rH = std::atoi(option.c_str());
          graphics._renderResolutionHeight = rH;
      }
      line.clear();
    }
  }
}


void Engine::readUserConfigs( UserConfigParams& params )
{
  FileHandle Buf;
  FilesystemResult result =
      gFilesystem().ReadFrom("Configs/UserConfigs.recluse", &Buf);
  if (result == FilesystemResult_NotFound) {
    Log(rWarning) << "UserConfigs not found! Setting default user "
                     "configuration.\n";
    return;
  }

  std::string line = "";
  for (size_t i = 0; i < Buf.Sz; ++i) {
    TChar ch = Buf.Buf[i];
    line.push_back(ch);
    if (ch == '\n' || i == (Buf.Sz - 1)) {
      Log(rDebug) << line;
      if (availableOption(line, "MouseSensitivityX")) {
        std::string option = getOption( line );
        R32 s = std::stof( option ) / 10000.0f;
        params._mouseSensitivityX = Clamp(s, 0.0001f, 1.0f);
      }
      if (availableOption(line, "MouseSensitivityY")) {
        std::string option = getOption( line );
        R32 s = std::stof( option ) / 10000.0f;
        params._mouseSensitivityY = Clamp(s, 0.0001f, 1.0f);
      }
      if ( availableOption( line, "FieldOfView" ) ) {
        std::string option = getOption( line );
        R32 fov = std::stof( option );
        params._fieldOfView = Radians(fov - 20.0);
      }
      if (availableOption(line, "Multithreading")) {
        std::string option = getOption(line);
        if ( option.compare("true") == 0) {
          params._engineMultiThreading = true;
        } else {
          params._engineMultiThreading = false;
        }
      }
      if (availableOption(line, "CameraShakeQuality")) {
        std::string option = getOption(line);
        if (option.compare("high") == 0) {
          params._cameraShakeQuality = USER_QUALITY_HIGH;
        } else if ( option.compare("medium") == 0 ) {
          params._cameraShakeQuality = USER_QUALITY_MEDIUM;
        } else if ( option.compare("low") == 0 ) {
          params._cameraShakeQuality = USER_QUALITY_LOW;
        } else {
          params._cameraShakeQuality = USER_QUALITY_NONE;
        }
      }
      if (availableOption(line, "WindowResolutionWidth")) {
          std::string option = getOption(line);
          params._windowWidth = atoi(option.c_str());
      }
      if (availableOption(line, "WindowResolutionHeight")) {
          std::string option = getOption(line);
          params._windowHeight = atoi(option.c_str());
      }
      if (availableOption(line, "Window")) {
          std::string option = getOption(line);
          if (option.compare("borderless") == 0) {
              params._windowType = WindowType_Borderless;
          }
          else if (option.compare("fullscreen") == 0) {
              params._windowType = WindowType_Fullscreen;
          }
          else if (option.compare("border") == 0) {
              params._windowType = WindowType_Border;
          }
      }

      line.clear();
    }
  }

  if (params._windowWidth == 0) {
      params._windowWidth = 800;
  }

  if (params._windowHeight == 0) {
      params._windowHeight = 600;
  }
}
} // Recluse