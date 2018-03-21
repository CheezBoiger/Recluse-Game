// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Engine.hpp"
#include "RendererComponent.hpp"
#include "MaterialComponent.hpp"
#include "LightComponent.hpp"

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


Camera* Camera::GetMain()
{
  return gEngine().GetCamera();
}


Engine& gEngine()
{
  static Engine engine;
  return engine;
}


Engine::Engine()
  : m_pCamera(nullptr)
  , m_pPushedScene(nullptr)
  , m_GameMouseX(0.0)
  , m_GameMouseY(0.0)
  , m_SceneObjectCount(0)
  , m_pControlInputFunc(nullptr)
  , m_Running(false)
  , m_Stopping(false)
  , m_dLag(0.0)
{
  m_RenderCmdList.SetSortFunc([] (RenderCmd& cmd1, RenderCmd& cmd2) -> bool {
    if (!cmd1._pTarget || !cmd2._pTarget) return false;
    if (!cmd1._pTarget->Renderable || !cmd2._pTarget->Renderable) return false;
    Camera* camera = Camera::GetMain();
    MeshDescriptor* mesh1 = cmd1._pTarget->GetMeshDescriptor();
    MeshDescriptor* mesh2 = cmd2._pTarget->GetMeshDescriptor();
    Matrix4 m1 = mesh1->ObjectData()->_Model;
    Matrix4 m2 = mesh2->ObjectData()->_Model;
  
    Vector3 cam_pos = camera->Position();
    Vector3 v1 = Vector3(m1[3][0], m1[3][1], m1[3][2]) - cam_pos;
    Vector3 v2 = Vector3(m2[3][0], m2[3][1], m2[3][2]) - cam_pos;
    
    return v1.Magnitude() < v2.Magnitude();
  });
}


Engine::~Engine()
{
}


void Engine::StartUp(std::string appName, b8 fullscreen, i32 width, i32 height, const GraphicsConfigParams* params)
{
  if (m_Running) return;

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

  m_Window.Create(appName, width, height);
  gRenderer().Initialize(&m_Window, params);

  gRenderer().PushCmdList(&m_RenderCmdList);

  Material::InitializeDefault();
  LightComponent::Initialize();

  if (fullscreen) {
    m_Window.SetToFullScreen();
  } else {
    m_Window.SetToCenter();
  }
}


void Engine::CleanUp()
{
  if (m_Running) return;
  gCore().ThrPool().StopAll();
  if (!m_Window.ShouldClose()) {
    m_Window.Close();
    Window::PollEvents();
  }

  LightComponent::CleanUp();
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
  m_Running = false;
}


void Engine::Run()
{
  if (m_Running) return;
  // TODO(): Signal to continue thread works.

  // Start up the time as the engine begins running.
  Time::Start();
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
  r64 dt = Time::DeltaTime * Time::ScaleTime;
  m_dLag += Time::DeltaTime;

  while (m_dLag >= Time::FixTime) {
    gAnimation().UpdateState(dt);
    gUI().UpdateState(dt);
#if !defined FORCE_AUDIO_OFF
    gAudio().UpdateState(dt);
#endif
#if !defined FORCE_PHYSICS_OFF
    gPhysics().UpdateState(dt);
#endif
    UpdateGameLogic();
    m_dLag -= Time::FixTime;
  }

  SortCmdLists();
  gRenderer().Render();
}


void UpdateGameObject(Engine* engine, GameObject* object, size_t currNum)
{
  // Perform updates to the game object.
  //
  // TODO(): To Better optimize transform calculations for our render objects, we will need to distribute our 
  // rendering info in a separate memory block.
  //  
  object->Update();
}


void Engine::UpdateGameLogic()
{
  if (!m_pPushedScene) return;

  {
    DirectionalLight* pPrimary = m_pPushedScene->GetPrimaryLight();
    LightBuffer* pLights = gRenderer().LightData();
    pLights->_PrimaryLight._Ambient = pPrimary->_Ambient;
    pLights->_PrimaryLight._Color = pPrimary->_Color;
    pLights->_PrimaryLight._Direction = pPrimary->_Direction;
    pLights->_PrimaryLight._Enable = pPrimary->_Enable;
    pLights->_PrimaryLight._Intensity = pPrimary->_Intensity;
  }

  // Update camera and screen info.
  GlobalBuffer* gGlobalBuffer = gRenderer().GlobalData();
  if (m_pCamera) {
    m_pCamera->Update();
    m_pCamera->SetAspect(((r32)m_Window.Width() / (r32)m_Window.Height()));

    gGlobalBuffer->_CameraPos = Vector4(m_pCamera->Position(), 1.0f);
    gGlobalBuffer->_Proj = m_pCamera->Projection();
    gGlobalBuffer->_View = m_pCamera->View();
    gGlobalBuffer->_ViewProj = gGlobalBuffer->_View * gGlobalBuffer->_Proj;
    gGlobalBuffer->_InvView = gGlobalBuffer->_View.Inverse();
    gGlobalBuffer->_InvProj = gGlobalBuffer->_Proj.Inverse();
    gGlobalBuffer->_ScreenSize[0] = m_Window.Width();
    gGlobalBuffer->_ScreenSize[1] = m_Window.Height();
    gGlobalBuffer->_BloomEnabled = m_pCamera->Bloom();
    gGlobalBuffer->_Exposure = m_pCamera->Exposure();
    gGlobalBuffer->_Gamma = m_pCamera->Gamma();
    gGlobalBuffer->_MousePos = Vector2((r32)Mouse::X(), (r32)Mouse::Y());
    gGlobalBuffer->_fEngineTime = static_cast<r32>(Time::CurrentTime());
    gGlobalBuffer->_fDeltaTime = static_cast<r32>(Time::DeltaTime);

    m_CamFrustum.Update();
    gGlobalBuffer->_LPlane = m_CamFrustum._Planes[CCamViewFrustum::PLEFT];
    gGlobalBuffer->_RPlane = m_CamFrustum._Planes[CCamViewFrustum::PRIGHT];
    gGlobalBuffer->_TPlane = m_CamFrustum._Planes[CCamViewFrustum::PTOP];
    gGlobalBuffer->_BPlane = m_CamFrustum._Planes[CCamViewFrustum::PBOTTOM];
    gGlobalBuffer->_NPlane = m_CamFrustum._Planes[CCamViewFrustum::PNEAR];
    gGlobalBuffer->_FPlane = m_CamFrustum._Planes[CCamViewFrustum::PFAR];
  }

  for ( u32 i = 0; i < m_SceneObjectCount; ++i ) {
    GameObject* object = m_cachedGameObjects[i];
    if ( object ) {
      object->Update();
    }
  }

 //TraverseScene(UpdateGameObject);
}


void BuildSceneCallback(Engine* engine, GameObject* object, size_t currNum)
{ 
  CmdList& list = engine->RenderCommandList();
  auto&     cache = engine->GetGameObjectCache();
  // Perform updates to the game object.
  RendererComponent* render = object->GetComponent<RendererComponent>();
  if (render) {
    list[currNum]._pTarget = render->RenderObj();
  }
  
  cache[currNum] = object;
}


void Engine::BuildScene()
{
  if (!m_pPushedScene) return;
  m_RenderCmdList.Clear();
  m_RenderCmdList.Resize(gGameObjectManager().NumOccupied());
  m_cachedGameObjects.resize(gGameObjectManager().NumOccupied());
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


void Engine::SortCmdLists()
{
  m_RenderCmdList.Sort();
}
} // Recluse