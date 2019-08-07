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


void KeyCallback(Window* window, i32 key, i32 scanCode, i32 action, i32 mods)
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


void WindowResized(Window* window, i32 width, i32 height)
{
  if ( gRenderer( ).isActive( ) && gRenderer( ).isInitialized( ) ) 
  {
    Mouse::resetButtonActions( );
    gRenderer( ).updateRendererConfigs( nullptr );
    R_DEBUG( rVerbose, "Engine Renderer has been updated prior to resize.\n" );
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


void Engine::processInput() 
{
  Window::pollEvents();
  if (m_pControlInputFunc) m_pControlInputFunc();
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


  // Update using next frame input.
  gUI().updateState(dt);

  AnimationComponent::updateComponents();
  gAnimation().updateState(dt);
  
  PhysicsComponent::UpdateFromPreviousGameLogic();
  traverseScene(UpdateTransform);
  updateSunLight();

  m_workers[0] = std::thread([&] () -> void {
    gPhysics().updateState(dt, tick);
    PhysicsComponent::updateComponents();

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



std::string getOption(const std::string& line) 
{
  size_t pos = line.find('=');
  if (pos == std::string::npos) return "";
  std::string option = line.substr(pos + 1);
  option.erase(std::remove_if(option.begin(), option.end(),
                              [](u8 x) -> i32 { return std::isspace(x); }),
               option.end());
  std::transform(option.begin(), option.end(), option.begin(), std::tolower);
  return option;
}


b32 availableOption(const std::string& line, const tchar* option) 
{
  size_t pos = line.find(option);
  if (pos != std::string::npos) return true;
  return false;
}


GraphicsConfigParams Engine::readGraphicsConfig( u32& w, u32&h )
{
  GraphicsConfigParams graphics = kDefaultGpuConfigs;
  FileHandle Buf;
  FilesystemResult result =
      gFilesystem().ReadFrom("Configs/RendererConfigs.recluse", &Buf);
  if (result == FilesystemResult_NotFound) {
    Log(rWarning) << "RendererConfigs not found! Setting default rendering "
                     "configuration.\n";
    return graphics;
  }

  std::string line = "";
  for (size_t i = 0; i < Buf.Sz; ++i) {
    tchar ch = Buf.Buf[i];
    line.push_back(ch);
    if (ch == '\n') {
      std::cout << line;
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
          graphics._EnableSoftShadows = true;
        } else {
          graphics._EnableSoftShadows = false;
        }
      }
      if (availableOption(line, "Multithreading")) {
        std::string option = getOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableMultithreadedRendering = true;
        } else {
          graphics._EnableMultithreadedRendering = false;
        }
      }
      if ( availableOption( line, "ShadowMapResolution" ) ) {
        std::string option = getOption( line );
        u32 res = atoi( option.c_str( ) );
        graphics._shadowMapRes = res;
      }
      if (availableOption(line, "ShadowMapPointLightResolution")) {
        std::string option = getOption(line);
        u32 res = atoi(option.c_str());
        graphics._shadowMapArrayRes = res;
      }
      if (availableOption(line, "RenderResolution")) {
        std::string option = getOption(line);
        if (option.compare("800x600") == 0) {
          graphics._Resolution = Resolution_800x600;
        } else if (option.compare("1200x800") == 0) {
          graphics._Resolution = Resolution_1200x800;
        } else if (option.compare("1280x720") == 0) {
          graphics._Resolution = Resolution_1280x720;
        } else if (option.compare("1440x900") == 0) {
          graphics._Resolution = Resolution_1440x900;
        } else if (option.compare("1920x1080") == 0) {
          graphics._Resolution = Resolution_1920x1080;
        } else if (option.compare("1920x1200") == 0) {
          graphics._Resolution = Resolution_1920x1200;
        } else if (option.compare("2048x1440") == 0) {
          graphics._Resolution = Resolution_2048x1440;
        } else if (option.compare("3840x2160") == 0) {
          graphics._Resolution = Resolution_3840x2160;
        } else if (option.compare("7680x4320") == 0) {
          graphics._Resolution = Resolution_7680x4320;
        } else if (option.compare("unknown") == 0) {
        }
      }
      if (availableOption(line, "WindowResolutionX")) {
        std::string option = getOption(line);
        w = atoi(option.c_str());
      }
      if (availableOption(line, "WindowResolutionY")) {
        std::string option = getOption(line);
        h = atoi(option.c_str());
      }
      if (availableOption(line, "Window")) {
        std::string option = getOption(line);
        if (option.compare("borderless") == 0) {
          graphics._WindowType = WindowType_Borderless;
        } else if (option.compare("fullscreen") == 0) {
          graphics._WindowType = WindowType_Fullscreen;
        } else if (option.compare("border") == 0) {
          graphics._WindowType = WindowType_Border;
        }
      }
      line.clear();
    }
  }

  if (w == 0) {
    w = 800;
  }

  if (h == 0) {
    h = 600;
  }

  return graphics;
}
} // Recluse