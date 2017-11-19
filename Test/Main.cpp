// Copyright (c) Recluse Project. All rights reserved.
#include "Game/Engine.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/Vertex.hpp"
#include "Renderer/UserParams.hpp"
#include "Renderer/CmdList.hpp"
#include "Renderer/RenderCmd.hpp"
#include "Renderer/RenderObject.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/TextureType.hpp"
#include "Core/Utility/Image.hpp"

#include <stdio.h>

using namespace Recluse;

bool noAlbedo2 = false;
bool noAlbedo = false;
static i32 keys[256];

void KeyCallback(Window* window, i32 key, i32 scanCode, i32 action, i32 mods)
{
  keys[key] = action;

  if (keys[KEY_CODE_2] == KEY_DOWN) Mouse::EnableMouse(!Mouse::Enabled());
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
  if (camera) {
    camera->Look(x, y);
  }
}


// TODO(): This needs to go into the engine as a member function, or it might be 
// better off a global?
void ProcessInput()
{
  Camera* camera = gEngine().GetCamera();
  Window* window = gEngine().GetWindow();

  if (keys[KEY_CODE_W] == KEY_DOWN) { camera->Move(Camera::FORWARD, Time::DeltaTime); }
  if (keys[KEY_CODE_S] == KEY_DOWN) { camera->Move(Camera::BACK, Time::DeltaTime); }
  if (keys[KEY_CODE_D] == KEY_DOWN) { camera->Move(Camera::LEFT, Time::DeltaTime); }
  if (keys[KEY_CODE_A] == KEY_DOWN) { camera->Move(Camera::RIGHT, Time::DeltaTime); }

  // Test Gamma correction
  if (keys[KEY_CODE_G] == KEY_DOWN) { gRenderer().SetGamma(gRenderer().Gamma() + (r32)(5.0 * Time::DeltaTime)); }
  if (keys[KEY_CODE_H] == KEY_DOWN) { gRenderer().SetGamma(gRenderer().Gamma() <= 0.0f ? 0.1f : gRenderer().Gamma() - (r32)(5.0 * Time::DeltaTime)); }
  // Test HDR Reinhard exposure.
  if (keys[KEY_CODE_E] == KEY_DOWN) { gRenderer().SetExposure(gRenderer().Exposure() + (r32)(3.0 * Time::DeltaTime)); }
  if (keys[KEY_CODE_R] == KEY_DOWN) { gRenderer().SetExposure(gRenderer().Exposure() - (r32)(3.0 * Time::DeltaTime)); }
  // Test albedo enabling.
  if (keys[KEY_CODE_V] == KEY_DOWN) { noAlbedo2 = !noAlbedo2; }
  if (keys[KEY_CODE_C] == KEY_DOWN) { noAlbedo = !noAlbedo; }

  if (keys[KEY_CODE_0] == KEY_DOWN) { gRenderer().EnableHDR(false); }
  if (keys[KEY_CODE_1] == KEY_DOWN) { gRenderer().EnableHDR(true); }

  // Window changing sets.
  if (keys[KEY_CODE_M] == KEY_DOWN) { window->SetToFullScreen(); }
  if (keys[KEY_CODE_N] == KEY_DOWN) { window->SetToWindowed(1200, 800); window->Show(); }
  if (keys[KEY_CODE_B] == KEY_DOWN) { window->SetToWindowed(800, 600, true); window->Show(); }
  if (keys[KEY_CODE_ESCAPE] == KEY_DOWN) { window->Close(); }
}


int main(int c, char* argv[])
{
  Log::DisplayToConsole(true);
  Mouse::EnableMouse(false);

  gEngine().StartUp(RTEXT("私は猫が大好き"), false, 1200, 800);
  Window::SetKeyboardCallback(KeyCallback);
  Window::SetWindowResizeCallback(WindowResized);
  Window::SetMousePositionCallback(MousePositionMove);

  Window* window = gEngine().GetWindow(); 
  window->SetToWindowed(1200, 800);
  window->SetToCenter();
  window->Show();    

  printf("App directory: %s\n", gFilesystem().CurrentAppDirectory());
  ///////////////////////////////////////////////////////////////////////////////////////
  // build the scene for the render. Should our cmd list be updated, you need to call
  // this function to update the scene. This is usually not hardcoded like this, as it 
  // is supposed to demonstrate how you can build a mesh and material outside the game 
  // loop.
  ///////////////////////////////////////////////////////////////////////////////////////
  Camera camera(Camera::PERSPECTIVE, Radians(55.0f), ((r32)window->Width() / (r32)window->Height()), 0.0001f, 1000.0f, 
    Vector3(-4.0f, 4.0f, -4.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3::UP);

  FirstPersonCamera fpsCamera(camera.FoV(), camera.Aspect(), camera.Near(), camera.Far(),
    Vector3(0.0f, 0.0f, -4.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3::UP);

  Log(rVerbose) << "Global camera created, attaching to engine.\n";
  gEngine().SetCamera(&fpsCamera);
  Camera* gCamera = gEngine().GetCamera();

  // Only thing we worry about is setting up lights.
  LightBuffer* lights = gEngine().LightData();
  Vector3 light0Pos = Vector3(-3.0f, 2.0f, 0.0f);
  lights->primaryLight.direction = Vector4(1.0f, 0.0f, 1.0f, 1.0f);
  lights->primaryLight.intensity = 0.5f;
  lights->primaryLight.color = Vector4(0.5f, 0.5f, 0.2f, 1.0f);
  lights->primaryLight.enable = true;

  lights->pointLights[0].enable = true;
  lights->pointLights[0].position = Vector4(light0Pos, 1.0f);
  lights->pointLights[0].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  lights->pointLights[0].range = 90.0f;

  lights->pointLights[1].enable = true;
  lights->pointLights[1].position = Vector4(3.0f, 2.0f, -4.0f, 1.0f);
  lights->pointLights[1].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  lights->pointLights[1].range = 30.0f;

  Image img;
  img.Load("box.jpg");
  Texture2D* albedo = gRenderer().CreateTexture2D();
  albedo->Initialize(img.Width(), img.Height());
  albedo->Update(img);
  img.CleanUp();

  auto sphereData = UVSphere::MeshInstance(1.0f, 60, 60);
  auto sphereIndices = UVSphere::IndicesInstance((u32)sphereData.size(), 60, 60);
  MeshData* sphereMeshDat = gRenderer().CreateMeshData();
  sphereMeshDat->Initialize(sphereData.size(), sizeof(SkinnedVertex), sphereData.data(), true, sphereIndices.size(), sphereIndices.data());

  auto cubeData = Cube::MeshInstance();
  auto cubeIndices = Cube::IndicesInstance();
  MeshData* cubeMeshDat = gRenderer().CreateMeshData();
  cubeMeshDat->Initialize(cubeData.size(), sizeof(SkinnedVertex), cubeData.data(), true, cubeIndices.size(), cubeIndices.data());
 
  Material cubeMaterial;
  SkinnedMesh cubeMesh;
  cubeMesh.Initialize(&gRenderer(), cubeMeshDat);
  cubeMaterial.SetAlbedo(albedo);
  ObjectBuffer* cubeInfo = cubeMesh.ObjectData();
  cubeInfo->model = Matrix4::Translate(Matrix4::Identity(), Vector3(0.0f, 0.0f, 0.0f));
  cubeInfo->normalMatrix = cubeInfo->model.Inverse().Transpose();
  cubeInfo->normalMatrix[3][0] = 0.0f;
  cubeInfo->normalMatrix[3][1] = 0.0f;
  cubeInfo->normalMatrix[3][2] = 0.0f;
  cubeInfo->normalMatrix[3][3] = 1.0f;

  Material cubeMaterial2;
  SkinnedMesh cubeMesh2;
  cubeMesh2.Initialize(&gRenderer(), sphereMeshDat);
  cubeMaterial2.SetAlbedo(albedo);
  ObjectBuffer* cubeInfo2 = cubeMesh2.ObjectData();
  cubeInfo2->model = Matrix4::Rotate(Matrix4::Translate(Matrix4::Identity(), Vector3(-3.0f, 0.0f, 3.0f)), Radians(45.0f), Vector3(0.0f, 1.0f, 0.0f));
  cubeInfo2->normalMatrix = cubeInfo2->model.Inverse().Transpose();
  cubeInfo2->normalMatrix[3][0] = 0.0f;
  cubeInfo2->normalMatrix[3][1] = 0.0f;
  cubeInfo2->normalMatrix[3][2] = 0.0f;
  cubeInfo2->normalMatrix[3][3] = 1.0f;

  Material cubeMaterial3;
  SkinnedMesh cubeMesh3;
  cubeMesh3.Initialize(&gRenderer(), cubeMeshDat);
  cubeMaterial3.SetAlbedo(albedo);
  ObjectBuffer* cubeInfo3 = cubeMesh3.ObjectData();
  cubeInfo3->model = Matrix4::Scale(Matrix4(), Vector3(0.1f, 0.1f, 0.1f)) * Matrix4::Translate(Matrix4::Identity(), light0Pos);
  cubeInfo3->normalMatrix = cubeInfo3->model.Inverse().Transpose();
  cubeInfo3->normalMatrix[3][0] = 0.0f;
  cubeInfo3->normalMatrix[3][1] = 0.0f;
  cubeInfo3->normalMatrix[3][2] = 0.0f;
  cubeInfo3->normalMatrix[3][3] = 1.0f;


  RenderObject* obj1 = gRenderer().CreateRenderObject();
  RenderObject* obj2 = gRenderer().CreateRenderObject();
  RenderObject* obj3 = gRenderer().CreateRenderObject();

  obj1->materialId = &cubeMaterial;
  obj1->meshId = &cubeMesh;
  obj1->skinned = true;

  obj2->materialId = &cubeMaterial2;
  obj2->meshId = &cubeMesh2;
  obj2->skinned = true;

  obj3->materialId = &cubeMaterial3;
  obj3->meshId = &cubeMesh3;
  obj3->skinned = true;

  obj1->Initialize();
  obj2->Initialize();
  obj3->Initialize();

  ///////////////////////////////////////////////////////////////////////////////////////

  /* Create the cmd list to send to the renderer. */
  // TODO(): This part should be something our engine worries about, not the user.
  // Plan: Create Game Objects from Engine, this will give engine handle to all objects
  // in game.
  CmdList& list = gEngine().RenderCommandList();
  list.Resize(3);
  list[0].target = obj1;
  list[1].target = obj2;
  list[2].target = obj3;

  gRenderer().Build();

  r64 timeAccumulator = 0.0;
  ///////////////////////////////////////////////////////////////////////////////////////
  // Game loop...
  ///////////////////////////////////////////////////////////////////////////////////////
  Log(rVerbose) << "Entering game loop...\n";

  while (!window->ShouldClose()) {
    Time::Update();

    timeAccumulator += Time::DeltaTime;
    r64 dt = Time::DeltaTime * Time::ScaleTime;

    // Render out the scene.
    gAnimation().UpdateState(dt);
    gUI().UpdateState(dt);

    // TODO(): needs to be on separate thread.
    while (timeAccumulator > Time::FixTime) {
      // TODO(): Instead of sleeping, update the game state.
      timeAccumulator -= Time::FixTime;
    }

    // box 1 transforming.
    cubeInfo->model = Matrix4::Translate(Matrix4::Identity(), Vector3(0.0f, 0.0f, 0.0f));
    cubeInfo->normalMatrix = cubeInfo->model.Inverse().Transpose();
    cubeInfo->normalMatrix[3][0] = 0.0f;
    cubeInfo->normalMatrix[3][1] = 0.0f;
    cubeInfo->normalMatrix[3][2] = 0.0f;
    cubeInfo->normalMatrix[3][3] = 1.0f;

    // light cube transforming.
    light0Pos = Vector3(sinf((r32)Time::CurrentTime() * 1.0f) * -3.0f, 2.0f, 0.0f);
    lights->pointLights[0].position = Vector4(light0Pos, 1.0f);
    cubeInfo3->model = Matrix4::Scale(Matrix4(), Vector3(0.1f, 0.1f, 0.1f)) * 
      Matrix4::Rotate(Matrix4::Identity(), -Radians((r32)(Time::CurrentTime()) * 50.0f), Vector3(0.0f, 1.0f, 0.0f)) * 
      Matrix4::Translate(Matrix4::Identity(), light0Pos);

    cubeInfo3->normalMatrix = cubeInfo3->model.Inverse().Transpose();
    cubeInfo3->normalMatrix[3][0] = 0.0f;
    cubeInfo3->normalMatrix[3][1] = 0.0f;
    cubeInfo3->normalMatrix[3][2] = 0.0f;
    cubeInfo3->normalMatrix[3][3] = 1.0f;

    if (noAlbedo2) { cubeMesh2.ObjectData()->hasAlbedo = false; } else { cubeMesh2.ObjectData()->hasAlbedo = true; }
    if (noAlbedo) { cubeMesh.ObjectData()->hasAlbedo = false; } else { cubeMesh.ObjectData()->hasAlbedo = true; }

    // //////////
    // End updates.
    // //////////
    //Log(rDebug) << gCamera->Front() << "\n";

    // Syncronize engine modules, as they run on threads.
    gEngine().Update(dt);
    gCore().Sync();
    gRenderer().Render();

    r64 fps = SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime);
    //printf("window width=%d\t\theight=%d\t\t\r", window.Width(), window.Height());
    printf("%f ms\t\t%d fps\t\t\t\r", timeAccumulator * 1000.0, u32(fps));
    Window::PollEvents();
    ProcessInput();
  }

  gRenderer().WaitIdle();
  ///////////////////////////////////////////////////////////////////////////////////////
  // Free up resources that were allocated.
  ///////////////////////////////////////////////////////////////////////////////////////
  gRenderer().FreeTexture2D(albedo);
  gRenderer().FreeRenderObject(obj1);
  gRenderer().FreeRenderObject(obj2);
  gRenderer().FreeRenderObject(obj3);
  gRenderer().FreeMeshData(cubeMeshDat);
  gRenderer().FreeMeshData(sphereMeshDat);

  cubeMesh.CleanUp();
  cubeMesh2.CleanUp();
  cubeMesh3.CleanUp();
  ///////////////////////////////////////////////////////////////////////////////////////  
  gEngine().CleanUp();
#if (_DEBUG)
  printf("\nEngine modules cleaned up, Press Enter to continue...\n");
  getchar();
#endif
  return EXIT_SUCCESS;
}