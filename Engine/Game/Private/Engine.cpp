// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Engine.hpp"
#include "Scene/Scene.hpp"
#include "Core/Thread/CoreThread.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Renderer/RenderObject.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/Material.hpp"
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
  Log() << "Button: " << ((button == Mouse::LEFT) ? "Left" : "Right");
  Log() << " pressed at location: X: " << Mouse::X() << " Y: " << Mouse::Y() << "\n"; 
  
}


Engine& gEngine()
{
  static Engine engine;
  return engine;
}


Engine::Engine()
  : mCamera(nullptr)
  , mCamMat(nullptr)
  , mLightMat(nullptr)
  , m_GameMouseX(0.0)
  , m_GameMouseY(0.0)
{
}


Engine::~Engine()
{
}


void Engine::StartUp(std::string appName, b8 fullscreen, i32 width, i32 height)
{
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

  mWindow.Create(appName, width, height);

  gRenderer().Initialize(&mWindow);

  mCamMat = gRenderer().CreateGlobalMaterial();
  mLightMat = gRenderer().CreateLightMaterial();
  
  mCamMat->Initialize();
  mCamMat->Update();

  gRenderer().SetGlobalMaterial(mCamMat);
  gRenderer().PushCmdList(&mRenderCmdList);

  if (fullscreen) {
    mWindow.SetToFullScreen();
    mWindow.Show();
  } else {
    mWindow.SetToCenter();
    mWindow.Show();
  }
}


void Engine::CleanUp()
{
  gRenderer().FreeGlobalMaterial(mCamMat);

  mCamMat = nullptr;
  mLightMat = nullptr;

  gUI().ShutDown();
  gAnimation().ShutDown();
  gRenderer().ShutDown();
  gFilesystem().ShutDown();
  gCore().ShutDown();
}


void Engine::Update(r64 dt)
{
  // Update camera and screen info.
  GlobalBuffer* gCamBuffer = mCamMat->Data();
  if (mCamera) {
    mCamera->Update();
    mCamera->SetAspect(((r32)mWindow.Width() / (r32)mWindow.Height()));

    gCamBuffer->cameraPos = Vector4(mCamera->Position(), 1.0f);
    gCamBuffer->proj = mCamera->Projection();
    gCamBuffer->view = mCamera->View();
    gCamBuffer->viewProj = gCamBuffer->view * gCamBuffer->proj;
    gCamBuffer->screenSize[0] = mWindow.Width();
    gCamBuffer->screenSize[1] = mWindow.Height();
  }

  if (mCamMat) {
    mCamMat->Update();
  }

  if (mLightMat) {
    mLightMat->Update();
  }

  // TODO(): RenderObject updated, We need to use RenderObject Now.
  for (u32 i = 0; i < mRenderCmdList.Size(); ++i) {
    RenderCmd& cmd = mRenderCmdList[i];
    RenderObject* obj = cmd.target;

    if (obj && obj->MeshDescriptorId) {
      obj->MeshDescriptorId->Update();
    }
  }
}


void Engine::SetLightData(LightMaterial* lights)
{
  if (lights) {
    gRenderer().SetLightMaterial(lights);
    mLightMat = lights;
  } else {
    Log(rError) << "Null lights passed... using previous light data.";
  }
}


void Engine::ProcessInput()
{

}
} // Recluse