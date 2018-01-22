// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Engine.hpp"
#include "RendererComponent.hpp"

#include "Scene/Scene.hpp"
#include "Core/Thread/CoreThread.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Renderer/RenderObject.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/UserParams.hpp"

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
  Camera* camera = gEngine().GetCamera();
  if (camera && !Mouse::Enabled()) {
    camera->Look(x, y);
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
  : m_pCamera(nullptr)
  , m_pLights(nullptr)
  , m_pPushedScene(nullptr)
  , m_GameMouseX(0.0)
  , m_GameMouseY(0.0)
  , m_SceneObjectCount(0)
  , m_pControlInputFunc(nullptr)
  , m_Running(false)
  , m_Stopping(false)
{
}


Engine::~Engine()
{
}


void Engine::StartUp(std::string appName, b8 fullscreen, i32 width, i32 height)
{
  if (m_Running) return;

  // NOTE(): Always start up the core first, before starting anything else up.
  gCore().StartUp();
  gFilesystem().StartUp();
  gRenderer().StartUp();
  gAnimation().StartUp();
  gUI().StartUp();

  Window::SetKeyboardCallback(KeyCallback);
  Window::SetWindowResizeCallback(WindowResized);
  Window::SetMousePositionCallback(MousePositionMove);
  Window::SetMouseButtonCallback(MouseButtonClick);

  m_Window.Create(appName, width, height);
  gRenderer().Initialize(&m_Window);

  // Set up lights.
  m_pLights = gRenderer().CreateLightDescriptor();
  m_pLights->Initialize();

  gRenderer().SetLightDescriptor(m_pLights);

  gRenderer().PushCmdList(&m_RenderCmdList);

  if (fullscreen) {
    m_Window.SetToFullScreen();
  } else {
    m_Window.SetToCenter();
  }
}


void Engine::CleanUp()
{
  if (m_Running) return;
  if (!m_Window.ShouldClose()) {
    m_Window.Close();
    Window::PollEvents();
  }

  if (m_pLights) {
    gRenderer().FreeLightDescriptor(m_pLights);
    m_pLights = nullptr;
  }
  gUI().ShutDown();
  gAnimation().ShutDown();
  gRenderer().ShutDown();
  gFilesystem().ShutDown();
  gCore().ShutDown();
  m_Running = false;
}


void Engine::Run()
{
  if (m_Running) return;
  // TODO(): Signal to continue thread works.
  m_Running = true;
}


void Engine::Stop()
{
  if (!m_Running) return;
  // TODO(): Signal to stop thread works.

  gRenderer().WaitIdle();
  m_Running = false;
}


void Engine::Update()
{
  if (m_Window.ShouldClose() || m_Stopping) {
    Stop();
    return;
  }
  // Render out the scene.
  r64 tick = Time::DeltaTime * Time::ScaleTime;
  gAnimation().UpdateState(tick);
  gUI().UpdateState(tick);
  UpdateGameLogic();

  m_TimeAccumulate += Time::DeltaTime;
  // TODO(): needs to be on separate thread.
  while (m_TimeAccumulate >= Time::FixTime) {
    // TODO(): Instead of doing nothing, update the game state.
    // Game objects have FixedUpdate() that may be used for a fixed tick
    // calc.
    m_TimeAccumulate -= Time::FixTime;
    
  }

  // Update camera and screen info.
  GlobalBuffer* gCamBuffer = gRenderer().GlobalData();
  if (m_pCamera) {
    m_pCamera->Update();
    m_pCamera->SetAspect(((r32)m_Window.Width() / (r32)m_Window.Height()));

    gCamBuffer->_CameraPos = Vector4(m_pCamera->Position(), 1.0f);
    gCamBuffer->_Proj = m_pCamera->Projection();
    gCamBuffer->_View = m_pCamera->View();
    gCamBuffer->_ViewProj = gCamBuffer->_View * gCamBuffer->_Proj;
    gCamBuffer->_ScreenSize[0] = m_Window.Width();
    gCamBuffer->_ScreenSize[1] = m_Window.Height();
    gCamBuffer->_BloomEnabled = m_pCamera->Bloom();
    gCamBuffer->_Exposure = m_pCamera->Exposure();
    gCamBuffer->_Gamma = m_pCamera->Gamma();
    gCamBuffer->_MousePos = Vector2((r32)Mouse::X(), (r32)Mouse::Y());
    gCamBuffer->_EnableShadows = m_pLights->PrimaryShadowEnabled();
    gCamBuffer->_EnableAA = m_pCamera->AA();

    m_CamFrustum.Update();
    gCamBuffer->_LPlane = m_CamFrustum._Planes[CCamViewFrustum::PLEFT];
    gCamBuffer->_RPlane = m_CamFrustum._Planes[CCamViewFrustum::PRIGHT];
    gCamBuffer->_TPlane = m_CamFrustum._Planes[CCamViewFrustum::PTOP];
    gCamBuffer->_BPlane = m_CamFrustum._Planes[CCamViewFrustum::PBOTTOM];
    gCamBuffer->_NPlane = m_CamFrustum._Planes[CCamViewFrustum::PNEAR];
    gCamBuffer->_FPlane = m_CamFrustum._Planes[CCamViewFrustum::PFAR];
  }

  if (m_pLights) {
    if (m_pCamera) m_pLights->SetViewerPosition(m_pCamera->Position());
    m_pLights->Update();
  }

  gCore().Sync();
  gRenderer().Render();
}


void Engine::UpdateRenderObjects()
{
  // TODO(): RenderObject updated, We need to use RenderObject Now.
  for (u32 i = 0; i < m_RenderCmdList.Size(); ++i) {
    RenderCmd& cmd = m_RenderCmdList[i];
    RenderObject* obj = cmd._pTarget;

    if (obj && obj->MeshDescriptorId) {
      obj->MaterialId->Update();
      obj->MeshDescriptorId->Update();
    }
  }
}


void UpdateGameObject(Engine* engine, GameObject* object, size_t currNum)
{
  // Perform updates to the game object.
  //
  // TODO(): Need to multithread this!
  //  
  object->Update();
}


void Engine::UpdateGameLogic()
{
  if (!m_pPushedScene) return;

  {
    DirectionalLight* pPrimary = m_pPushedScene->GetPrimaryLight();
    LightBuffer* pLights = m_pLights->Data();
    pLights->_PrimaryLight._Ambient = pPrimary->_Ambient;
    pLights->_PrimaryLight._Color = pPrimary->_Color;
    pLights->_PrimaryLight._Direction = pPrimary->_Direction;
    pLights->_PrimaryLight._Enable = pPrimary->_Enable;
    pLights->_PrimaryLight._Intensity = pPrimary->_Intensity;
  }

  TraverseScene(UpdateGameObject);
}


void BuildSceneCallback(Engine* engine, GameObject* object, size_t currNum)
{ 
  CmdList& list = engine->RenderCommandList();
  // Perform updates to the game object.
  RendererComponent* render = object->GetComponent<RendererComponent>();
  if (render) {
    list[currNum]._pTarget = render->RenderObj();
  }
}


void Engine::BuildScene()
{
  if (!m_pPushedScene) return;
  m_RenderCmdList.Clear();
  m_RenderCmdList.Resize(2056);
  TraverseScene(BuildSceneCallback);
  gRenderer().Build();
}


void Engine::TraverseScene(GameObjectActionCallback callback)
{
  // Traversing the scene graph using DFS.
  // TODO(): Probably want to make a real stack allocator that doesn't have a terrible
  // amortized time complex like this vector.
  std::vector<GameObject*> nodes;
  m_SceneObjectCount = 0;
  SceneNode* root = m_pPushedScene->GetRoot();

  for (size_t i = 0; i < root->GetChildCount(); ++i) {
    nodes.push_back(root->GetChild(i));
  }
  
  while (!nodes.empty()) {
    GameObject* object = nodes.back();
    nodes.pop_back();

    callback(this, object, m_SceneObjectCount);
    m_SceneObjectCount++;

    // Now query its children.
    size_t child_count = object->GetChildrenCount();
    for (size_t i = 0; i < child_count; ++i) {
      GameObject* child = object->GetChild(i);
      nodes.push_back(child);
    }
  }
}

} // Recluse