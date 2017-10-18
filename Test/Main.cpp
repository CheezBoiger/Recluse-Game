// Copyright (c) Recluse Project. All rights reserved.
#include "Game/Engine.hpp"
#include "Game/Geometry/Cube.hpp"

#include "Renderer/Vertex.hpp"
#include "Renderer/UserParams.hpp"
#include "Renderer/CmdList.hpp"
#include "Renderer/RenderCmd.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/Material.hpp"

#include <stdio.h>

using namespace Recluse;

void KeyCallback(Window* window, i32 key, i32 scanCode, i32 action, i32 mods)
{
  static i32 keys[256];
  keys[key] = action; 
  
  if (keys[KEY_CODE_D] == KEY_DOWN) window->SetToFullScreen();
  if (keys[KEY_CODE_A] == KEY_DOWN) { window->SetToWindowed(1200, 800); window->Show(); }
  if (keys[KEY_CODE_W] == KEY_DOWN) { window->SetToWindowed(800, 600, true); window->Show(); }
  if (keys[KEY_CODE_ESCAPE] == KEY_DOWN) window->Close();
}


void WindowResized(Window* window, i32 width, i32 height)
{
  if (gRenderer().IsActive() && gRenderer().Initialized()) {
    UserParams params;
    gRenderer().UpdateRendererConfigs(&params);
  }
}

int main(int c, char* argv[])
{
  // NOTE(): Always start up the core first, before starting anything else up.
  gCore().StartUp();
  gFilesystem().StartUp();
  gRenderer().StartUp();
  gPhysics().StartUp();
  gAudio().StartUp();
  gAnimation().StartUp();
  gUI().StartUp();

  Window::SetKeyboardCallback(KeyCallback);
  Window::SetWindowResizeCallback(WindowResized);

  Window window;
  window.Create(RTEXT("私は猫が大好き"), 800, 600); 
  window.SetToWindowed(1200, 800);
  window.SetToCenter();
  window.Show();    

  gRenderer().Initialize(&window);
  printf("App directory: %s\n", gFilesystem().CurrentAppDirectory());
  ///////////////////////////////////////////////////////////////////////////////////////
  // build the scene for the render. Should our cmd list be updated, you need to call
  // this function to update the scene. This is usually not hardcoded like this, as it 
  // is supposed to demonstrate how you can build a mesh and material outside the game 
  // loop.
  ///////////////////////////////////////////////////////////////////////////////////////
  GlobalMaterial* globalMat = gRenderer().CreateGlobalMaterial(); 
  GlobalMaterial::GlobalBuffer* gBuffer = globalMat->Data();
  // TODO(): Some inconsistencies in the math!
  gBuffer->cameraPos = Vector4(0.0f, 1.0f, 1.0f, 1.0f);
  gBuffer->proj = Matrix4::Perspective(Radians(45.0f), ((r32)window.Width() / (r32)window.Height()), 0.0001f, 1000.0f);
  gBuffer->view = Matrix4::LookAt(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3::UP);
  gBuffer->viewProj = gBuffer->view * gBuffer->proj;
  globalMat->Initialize();
  globalMat->Update();

  LightMaterial* lightMat = gRenderer().CreateLightMaterial();
  lightMat->Initialize();
  lightMat->Update();

  auto cubeData = Cube::MeshInstance();
  auto cubeIndices = Cube::IndicesInstance();
  Mesh* cubeMesh = gRenderer().CreateMesh();
  cubeMesh->Initialize(cubeData.size(), sizeof(SkinnedVertex), cubeData.data(), true, cubeIndices.size(), cubeIndices.data());
 
  Material* cubeMaterial = gRenderer().CreateMaterial();
  Material::ObjectBuffer* cubeInfo = cubeMaterial->ObjectData();
  cubeInfo->hasAlbedo = false;
  cubeInfo->hasBones = false;
  cubeInfo->hasNormal = false;
  cubeInfo->hasMetallic = false;
  cubeInfo->hasRoughness = false;
  cubeInfo->hasAO = false;
  cubeInfo->hasEmissive = false;
  cubeInfo->model = Matrix4::Translate(Matrix4::Identity(), Vector3(0.0f, 0.0f, -2.0f));
  cubeInfo->normalMatrix = cubeInfo->model.Inverse().Transpose();
  cubeInfo->normalMatrix[3][0] = 0.0f;
  cubeInfo->normalMatrix[3][1] = 0.0f;
  cubeInfo->normalMatrix[3][2] = 0.0f;
  cubeInfo->normalMatrix[3][3] = 1.0f;

  cubeMaterial->Initialize();
  cubeMaterial->Update();     // 0x42

  ///////////////////////////////////////////////////////////////////////////////////////

  CmdList list;
  list.Resize(1);
  list[0].materialId = cubeMaterial;
  list[0].meshId = cubeMesh;

  gRenderer().PushCmdList(&list);
  gRenderer().SetGlobalMaterial(globalMat);
  gRenderer().SetLightMaterial(lightMat);
  gRenderer().Build();

  r64 timeAccumulator = 0.0;

  ///////////////////////////////////////////////////////////////////////////////////////
  // Game loop...
  ///////////////////////////////////////////////////////////////////////////////////////
  while (!window.ShouldClose()) {
    Time::Update();

    timeAccumulator += Time::DeltaTime;
    r64 dt = Time::DeltaTime * Time::ScaleTime;

    // Render out the scene.
    gAnimation().UpdateState(dt);
    gAudio().UpdateState(dt);
    gUI().UpdateState(dt);

    // TODO(): needs to be on separate thread.
    while (timeAccumulator > Time::FixTime) {
      // TODO(): Instead of sleeping, update the game state.
      gPhysics().UpdateState(dt);
      timeAccumulator -= Time::FixTime;
    }
    cubeInfo->model = Matrix4::Translate(Matrix4::Identity(), Vector3(0.0, Time::CurrentTime() * 0.5, -3.0f));
    cubeInfo->normalMatrix = cubeInfo->model.Inverse().Transpose();
    cubeInfo->normalMatrix[3][0] = 0.0f;
    cubeInfo->normalMatrix[3][1] = 0.0f;
    cubeInfo->normalMatrix[3][2] = 0.0f;
    cubeInfo->normalMatrix[3][3] = 1.0f;
    // Syncronize engine modules, as they run on threads.
    gCore().Sync();
    gRenderer().Render();

    r64 fps = SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime);
    //printf("window width=%d\t\theight=%d\t\t\r", window.Width(), window.Height());
    printf("%f ms\t\t%d fps\t\t\r", timeAccumulator * 1000.0, u32(fps));

    Window::PollEvents();
  }

  gRenderer().WaitIdle();
  ///////////////////////////////////////////////////////////////////////////////////////
  // Free up resources that were allocated.
  ///////////////////////////////////////////////////////////////////////////////////////

  gRenderer().FreeMaterial(cubeMaterial);
  gRenderer().FreeMesh(cubeMesh);

  gRenderer().FreeLightMaterial(lightMat);
  gRenderer().FreeGlobalMaterial(globalMat);

  ///////////////////////////////////////////////////////////////////////////////////////  
  gUI().ShutDown();
  gAnimation().ShutDown();
  gAudio().ShutDown();
  gRenderer().ShutDown();
  gPhysics().ShutDown();
  gFilesystem().ShutDown();
  gCore().ShutDown();
#if (_DEBUG)
  printf("\nEngine modules cleaned up, Press Enter to continue...\n");
  getchar();
#endif
  return EXIT_SUCCESS;
}