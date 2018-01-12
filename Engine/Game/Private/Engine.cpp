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
    UserParams params;
    gRenderer().UpdateRendererConfigs(&params);
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
    m_Window.Show();
  } else {
    m_Window.SetToCenter();
    m_Window.Show();
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
  gAnimation().UpdateState(Time::DeltaTime);
  gUI().UpdateState(Time::DeltaTime);

  m_TimeAccumulate += Time::DeltaTime;
  // TODO(): needs to be on separate thread.
  while (m_TimeAccumulate > Time::FixTime) {
    // TODO(): Instead of sleeping, update the game state.
    m_TimeAccumulate -= Time::FixTime;
    UpdateRenderObjects();
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
  if (m_pPushedScene) {
    R_DEBUG(rVerbose, "Updating game object transforms...\n");
    size_t SceneSz = m_pPushedScene->GameObjectCount();
    for (size_t i = 0; i < SceneSz; ++i) {
      GameObject* Obj = m_pPushedScene->Get(i);
      Transform* T = Obj->GetComponent<Transform>();
      if (!T) continue;

      RendererComponent* mesh = Obj->GetComponent<RendererComponent>();

      if (mesh) {
        MeshDescriptor* meshDescriptor = mesh->GetDescriptor();
        MaterialDescriptor* material = mesh->GetMaterial();
        ObjectBuffer* objectInfo = meshDescriptor->ObjectData();
        Vector3 position = T->Position + T->LocalPosition;
        Vector3 scale = T->LocalScale;
        Quaternion rotation = T->Rotation * T->LocalRotation;

        Matrix4 t = Matrix4::Translate(Matrix4::Identity(), position);
        Matrix4 r = rotation.ToMatrix4();
        Matrix4 s = Matrix4::Scale(Matrix4::Identity(), scale);

        objectInfo->_Model = s * r * t;
        objectInfo->_NormalMatrix = objectInfo->_Model.Inverse().Transpose();
        objectInfo->_NormalMatrix[3][0] = 0.0f;
        objectInfo->_NormalMatrix[3][1] = 0.0f;
        objectInfo->_NormalMatrix[3][2] = 0.0f;
        objectInfo->_NormalMatrix[3][3] = 1.0f;

        material->Update();
        meshDescriptor->Update();
      }
    }
  }

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
} // Recluse