// Copyright (c) Recluse Project. All rights reserved.
#include "Game/Engine.hpp"
#include "Game/Geometry/Cube.hpp"

#include "Renderer/Vertex.hpp"
#include "Renderer/UserParams.hpp"
#include "Renderer/CmdList.hpp"
#include "Renderer/RenderCmd.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/TextureType.hpp"
#include "Core/Utility/Image.hpp"

#include <stdio.h>

using namespace Recluse;
bool noAlbedo2 = false;
bool noAlbedo = false;

void KeyCallback(Window* window, i32 key, i32 scanCode, i32 action, i32 mods)
{
  static i32 keys[256];
  keys[key] = action; 
  
  // Test albedo enabling.
  if (keys[KEY_CODE_B] == KEY_DOWN) { noAlbedo2 = !noAlbedo2; }
  if (keys[KEY_CODE_N] == KEY_DOWN) { noAlbedo = !noAlbedo; }

  // Test Gamma correction
  if (keys[KEY_CODE_G] == KEY_DOWN) { gRenderer().SetGamma(gRenderer().Gamma() + (r32)(5.0 * Time::DeltaTime)); }
  if (keys[KEY_CODE_H] == KEY_DOWN) { gRenderer().SetGamma(gRenderer().Gamma() - (r32)(5.0 * Time::DeltaTime)); }
  // Test HDR Reinhard exposure.
  if (keys[KEY_CODE_E] == KEY_DOWN) { gRenderer().SetExposure(gRenderer().Exposure() + (r32)(5.0 * Time::DeltaTime)); }
  if (keys[KEY_CODE_R] == KEY_DOWN) { gRenderer().SetExposure(gRenderer().Exposure() - (r32)(5.0 * Time::DeltaTime)); }
  // Window changing sets.
  if (keys[KEY_CODE_D] == KEY_DOWN) { window->SetToFullScreen(); }
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


void MousePositionMove(Window* window, r64 x, r64 y)
{
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
  Window::SetMousePositionCallback(MousePositionMove);

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
  Vector3 camPosition = Vector3(-4.0f, 4.0f, -4.0f);
  gBuffer->cameraPos = Vector4(camPosition, 1.0f);
  gBuffer->proj = Matrix4::Perspective(Radians(45.0f), ((r32)window.Width() / (r32)window.Height()), 0.0001f, 1000.0f);
  gBuffer->view = Matrix4::LookAt(camPosition, Vector3(0.0f, 0.0f, 0.0f), Vector3::UP);
  gBuffer->viewProj = gBuffer->view * gBuffer->proj;
  globalMat->Initialize();
  globalMat->Update();

  LightMaterial* lightMat = gRenderer().CreateLightMaterial();
  lightMat->Initialize();
  lightMat->Update();

  Image img;
  img.Load("box.jpg");
  Texture2D* albedo = gRenderer().CreateTexture2D();
  albedo->Initialize(img);
  img.CleanUp();

  auto cubeData = Cube::MeshInstance();
  auto cubeIndices = Cube::IndicesInstance();
  Mesh* cubeMesh = gRenderer().CreateMesh();
  cubeMesh->Initialize(cubeData.size(), sizeof(SkinnedVertex), cubeData.data(), true, cubeIndices.size(), cubeIndices.data());
 
  Material* cubeMaterial = gRenderer().CreateMaterial();
  cubeMaterial->SetAlbedo(albedo);
  Material::ObjectBuffer* cubeInfo = cubeMaterial->ObjectData();
  cubeInfo->hasAlbedo = false;
  cubeInfo->hasBones = false;
  cubeInfo->hasNormal = false;
  cubeInfo->hasMetallic = false;
  cubeInfo->hasRoughness = false;
  cubeInfo->hasAO = false;
  cubeInfo->hasEmissive = false;
  cubeInfo->model = Matrix4::Translate(Matrix4::Identity(), Vector3(0.0f, 0.0f, 0.0f));
  cubeInfo->normalMatrix = cubeInfo->model.Inverse().Transpose();
  cubeInfo->normalMatrix[3][0] = 0.0f;
  cubeInfo->normalMatrix[3][1] = 0.0f;
  cubeInfo->normalMatrix[3][2] = 0.0f;
  cubeInfo->normalMatrix[3][3] = 1.0f;

  Material* cubeMaterial2 = gRenderer().CreateMaterial();
  cubeMaterial2->SetAlbedo(albedo);
  Material::ObjectBuffer* cubeInfo2 = cubeMaterial2->ObjectData();
  cubeInfo2->hasAlbedo = false;
  cubeInfo2->hasBones = false;
  cubeInfo2->hasNormal = false;
  cubeInfo2->hasMetallic = false;
  cubeInfo2->hasRoughness = false;
  cubeInfo2->hasAO = false;
  cubeInfo2->hasEmissive = false;
  cubeInfo2->model = Matrix4::Translate(Matrix4::Identity(), Vector3(-3.0f, 0.0f, 3.0f));
  cubeInfo2->normalMatrix = cubeInfo->model.Inverse().Transpose();
  cubeInfo2->normalMatrix[3][0] = 0.0f;
  cubeInfo2->normalMatrix[3][1] = 0.0f;
  cubeInfo2->normalMatrix[3][2] = 0.0f;
  cubeInfo2->normalMatrix[3][3] = 1.0f;

  cubeMaterial->Initialize();
  cubeMaterial->Update();     // 0x42

  cubeMaterial2->Initialize();
  cubeMaterial2->Update();

  ///////////////////////////////////////////////////////////////////////////////////////

  CmdList list;
  list.Resize(2);
  list[0].materialId = cubeMaterial;
  list[0].meshId = cubeMesh;

  list[1].materialId = cubeMaterial2;
  list[1].meshId = cubeMesh;

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

    // NOTE(): Update game state... This is hardcoded though.
    camPosition = Vector3(sinf((r32)Time::CurrentTime() * 0.5f) * 5.0f, 4.0f, -4.0f);
    gBuffer->cameraPos = camPosition;
    gBuffer->proj = Matrix4::Perspective(Radians(45.0f), ((r32)window.Width() / (r32)window.Height()), 0.0001f, 1000.0f);
    gBuffer->view = Matrix4::LookAt(camPosition, Vector3(0.0f, 0.0f, 0.0f), Vector3::UP);
    gBuffer->viewProj = gBuffer->view * gBuffer->proj;

    cubeInfo->model = Matrix4::Translate(Matrix4::Identity(), Vector3(0.0f, 0.0f, 0.0f));
    cubeInfo->normalMatrix = cubeInfo->model.Inverse().Transpose();
    cubeInfo->normalMatrix[3][0] = 0.0f;
    cubeInfo->normalMatrix[3][1] = 0.0f;
    cubeInfo->normalMatrix[3][2] = 0.0f;
    cubeInfo->normalMatrix[3][3] = 1.0f;
    if (noAlbedo2) { cubeMaterial2->ObjectData()->hasAlbedo = false; } else { cubeMaterial2->ObjectData()->hasAlbedo = true; }
    if (noAlbedo) { cubeMaterial->ObjectData()->hasAlbedo = false; } else { cubeMaterial->ObjectData()->hasAlbedo = true; }
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
  gRenderer().FreeTexture2D(albedo);
  gRenderer().FreeMaterial(cubeMaterial2);
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