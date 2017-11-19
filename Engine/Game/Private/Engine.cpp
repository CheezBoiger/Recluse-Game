// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Engine.hpp"
#include "Scene/Scene.hpp"
#include "Core/Thread/CoreThread.hpp"

#include "Core/Exception.hpp"
#include "Renderer/RenderObject.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/Material.hpp"


namespace Recluse {


Engine& gEngine()
{
  static Engine engine;
  return engine;
}


Engine::Engine()
  : mCamera(nullptr)
  , mCamMat(nullptr)
  , mLightMat(nullptr)
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

  mWindow.Create(appName, width, height);

  gRenderer().Initialize(&mWindow);

  mCamMat = gRenderer().CreateGlobalMaterial();
  mLightMat = gRenderer().CreateLightMaterial();
  
  mCamMat->Initialize();
  mCamMat->Update();

  mLightMat->Initialize();
  mLightMat->Update();

  gRenderer().SetGlobalMaterial(mCamMat);
  gRenderer().SetLightMaterial(mLightMat);
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
  gRenderer().FreeLightMaterial(mLightMat);

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

    if (obj && obj->meshId) {
      obj->meshId->Update();
    }
  }
}


void Engine::ProcessInput()
{

}
} // Recluse