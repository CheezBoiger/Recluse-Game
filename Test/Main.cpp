// Copyright (c) Recluse Project. All rights reserved.
#include "Game/Engine.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Game/CameraViewFrustum.hpp"
#include "Renderer/Vertex.hpp"
#include "Renderer/UserParams.hpp"
#include "Renderer/CmdList.hpp"
#include "Renderer/RenderCmd.hpp"
#include "Renderer/RenderObject.hpp"
#include "Renderer/MeshDescriptor.hpp"
#include "Renderer/MeshData.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/TextureType.hpp"
#include "Core/Utility/Image.hpp"

#include <stdio.h>
#include <array>
#include <random>

using namespace Recluse;

bool noAlbedo2 = false;
bool noAlbedo = false;

// TODO(): This just demonstrates key input. Normally you would use it for,
// say, moving a character and such.
void ProcessInput()
{
  Camera* camera = gEngine().GetCamera();
  Window* window = gEngine().GetWindow();

  if (Keyboard::KeyPressed(KEY_CODE_SHIFT)) { FirstPersonCamera* fpsCamera = reinterpret_cast<FirstPersonCamera*>(camera); fpsCamera->SetSpeed(200.0f); }
  if (Keyboard::KeyReleased(KEY_CODE_SHIFT)) { FirstPersonCamera* fpsCamera = reinterpret_cast<FirstPersonCamera*>(camera); fpsCamera->SetSpeed(5.0f); }
  if (Keyboard::KeyPressed(KEY_CODE_W)) { camera->Move(Camera::FORWARD, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_S)) { camera->Move(Camera::BACK, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_D)) { camera->Move(Camera::LEFT, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_A)) { camera->Move(Camera::RIGHT, Time::DeltaTime); }

  // Test Gamma correction
  if (Keyboard::KeyPressed(KEY_CODE_G)) { camera->SetGamma(camera->Gamma() + (r32)(5.0 * Time::DeltaTime)); }
  if (Keyboard::KeyPressed(KEY_CODE_H)) { camera->SetGamma(camera->Gamma() <= 0.0f ? 0.1f : camera->Gamma() - (r32)(5.0 * Time::DeltaTime)); }
  // Test HDR Reinhard exposure.
  if (Keyboard::KeyPressed(KEY_CODE_E)) { camera->SetExposure(camera->Exposure() + (r32)(2.0 * Time::DeltaTime)); }
  if (Keyboard::KeyPressed(KEY_CODE_R)) { camera->SetExposure(camera->Exposure() <= 0.0f ? 0.1f : camera->Exposure() - (r32)(2.0 * Time::DeltaTime)); }

  if (Keyboard::KeyPressed(KEY_CODE_0)) { camera->EnableBloom(false); }
  if (Keyboard::KeyPressed(KEY_CODE_1)) { camera->EnableBloom(true); }

  // Test albedo enabling.
  if (Keyboard::KeyPressed(KEY_CODE_V)) { noAlbedo2 = !noAlbedo2; }
  if (Keyboard::KeyPressed(KEY_CODE_C)) { noAlbedo = !noAlbedo; }

  // Window changing sets.
  if (Keyboard::KeyPressed(KEY_CODE_M)) { window->SetToFullScreen(); }
  if (Keyboard::KeyPressed(KEY_CODE_N)) { window->SetToWindowed(1200, 800); window->SetToCenter(); window->Show(); }
  if (Keyboard::KeyPressed(KEY_CODE_B)) { window->SetToWindowed(800, 600, true); window->SetToCenter(); window->Show(); }
  if (Keyboard::KeyPressed(KEY_CODE_ESCAPE)) { window->Close(); }
}

#define SPHERE_SEGS 64
#define PERFORMANCE_TEST 0


int main(int c, char* argv[])
{
  Log::DisplayToConsole(true);
  Mouse::Enable(false);
  Mouse::Show(false);

  gEngine().StartUp(RTEXT("Recluse"), false, 1200, 800);
  Window* window = gEngine().GetWindow();    

  printf("App directory: %s\n", gFilesystem().CurrentAppDirectory());
  ///////////////////////////////////////////////////////////////////////////////////////
  // build the scene for the render. Should our cmd list be updated, you need to call
  // this function to update the scene. This is usually not hardcoded like this, as it 
  // is supposed to demonstrate how you can build a mesh and material outside the game 
  // loop.
  ///////////////////////////////////////////////////////////////////////////////////////
  Camera camera(Camera::PERSPECTIVE, Radians(55.0f), ((r32)window->Width() / (r32)window->Height()), 0.0001f, 5000.0f, 
    Vector3(-4.0f, 4.0f, -4.0f), Vector3(0.0f, 0.0f, 1.0f));

  FirstPersonCamera fpsCamera(camera.FoV(), camera.Aspect(), camera.Near(), camera.Far(),
    Vector3(0.0f, 0.0f, -4.0f), Vector3(0.0f, 0.0f, -1.0f));

  fpsCamera.EnableFrustumCull(true);

  Log(rVerbose) << "Global camera created, attaching to engine.\n";
  gEngine().SetCamera(&fpsCamera);
  Camera* gCamera = gEngine().GetCamera();

  // Only thing we worry about is setting up lights.
  LightDescriptor* lightDesc = gRenderer().CreateLightDescriptor();
  lightDesc->Initialize();

  gEngine().SetLightData(lightDesc);

  LightBuffer* lights = lightDesc->Data();
  
  Vector3 light0Pos = Vector3(-3.0f, 2.0f, 0.0f);
  lights->primaryLight.direction = Vector4(1.0f, -1.0f, 1.0f, 1.0f);
  lights->primaryLight.intensity = 10.0f;
  lights->primaryLight.color = Vector4(0.8f, 0.8f, 0.5f, 1.0f);
  lights->primaryLight.enable = true;

  lights->directionalLights[0].enable = false;
  lights->directionalLights[0].direction = Vector4(-1.0f, 1.0f, -1.0f, 1.0f);
  lights->directionalLights[0].intensity = 5.0f;
  lights->directionalLights[0].color = Vector4(1.0f, 0.8f, 0.4f, 1.0f);

  lights->directionalLights[1].enable = false;
  lights->directionalLights[1].direction = Vector4(-1.0f, 0.0f, -1.0f, 1.0f);
  lights->directionalLights[1].intensity = 5.0f;
  lights->directionalLights[1].color = Vector4(1.0f, 0.8f, 0.4f, 1.0f);

  lights->directionalLights[2].enable = false;
  lights->directionalLights[2].direction = Vector4(1.0f, 0.0f, -1.0f, 1.0f);
  lights->directionalLights[2].intensity = 5.0f;
  lights->directionalLights[2].color = Vector4(1.0f, 0.8f, 0.4f, 1.0f);

  lights->pointLights[0].enable = true;
  lights->pointLights[0].position = Vector4(light0Pos, 1.0f);
  lights->pointLights[0].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  lights->pointLights[0].range = 10.0f;
  lights->pointLights[0].intensity = 1.0f;

  lights->pointLights[1].enable = false;
  lights->pointLights[1].position = Vector4(0.0f, 3.0f, 4.0f, 1.0f);
  lights->pointLights[1].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  lights->pointLights[1].range = 30.0f;

  Image img;
  img.Load("albedo.jpg");
  Texture2D* albedo = gRenderer().CreateTexture2D();
  albedo->Initialize(img.Width(), img.Height());
  albedo->Update(img);
  img.CleanUp();

  img.Load("normal.jpg");
  Texture2D* normal = gRenderer().CreateTexture2D();
  normal->Initialize(img.Width(), img.Height());
  normal->Update(img);
  img.CleanUp();

  auto sphereData = UVSphere::MeshInstance(1.0f, SPHERE_SEGS, SPHERE_SEGS);
  auto sphereIndices = UVSphere::IndicesInstance((u32)sphereData.size(), SPHERE_SEGS, SPHERE_SEGS);
  MeshData* sphereMeshDat = gRenderer().CreateMeshData();
  sphereMeshDat->Initialize(sphereData.size(), sizeof(StaticVertex), sphereData.data(), true, sphereIndices.size(), sphereIndices.data());

  auto cubeData = Cube::MeshInstance();
  auto cubeIndices = Cube::IndicesInstance();
  MeshData* cubeMeshDat = gRenderer().CreateMeshData();
  cubeMeshDat->Initialize(cubeData.size(), sizeof(StaticVertex), cubeData.data(), true, cubeIndices.size(), cubeIndices.data());

#if PERFORMANCE_TEST
#define ObjectCount 200
  r32 MaxNum = 150.0f;
  std::random_device gen;
  std::mt19937 r(gen());
  std::uniform_real_distribution<r32> uni(-MaxNum, MaxNum);
#endif
  Material cubeMaterial;
  MeshDescriptor cubeMesh;
  cubeMesh.Initialize(&gRenderer());
  cubeMaterial.SetAlbedo(albedo);
  cubeMaterial.SetNormal(normal);
  ObjectBuffer* cubeInfo = cubeMesh.ObjectData();
  cubeInfo->hasNormal = true;
  cubeInfo->model = Matrix4::Rotate(Matrix4::Identity(), Radians(90.0f), Vector3(0.0f, 1.0f, 0.0f)) * Matrix4::Translate(Matrix4::Identity(), Vector3(0.0f, 0.0f, 0.0f));
  cubeInfo->normalMatrix = cubeInfo->model.Inverse().Transpose();
  cubeInfo->normalMatrix[3][0] = 0.0f;
  cubeInfo->normalMatrix[3][1] = 0.0f;
  cubeInfo->normalMatrix[3][2] = 0.0f;
  cubeInfo->normalMatrix[3][3] = 1.0f;
  cubeInfo->baseMetal = 0.01f;
  cubeInfo->baseRough = 0.5f;

  Material cubeMaterial2;
  MeshDescriptor cubeMesh2;
  cubeMesh2.SetTransparent(false);
  cubeMesh2.Initialize(&gRenderer());
  cubeMaterial2.SetAlbedo(albedo);
  cubeMaterial2.SetNormal(normal);
  ObjectBuffer* cubeInfo2 = cubeMesh2.ObjectData();
  cubeInfo2->hasNormal = false;
  cubeInfo2->model = Matrix4::Rotate(Matrix4::Translate(Matrix4::Identity(), Vector3(-3.0f, 0.0f, 3.0f)), Radians(45.0f), Vector3(0.0f, 1.0f, 0.0f));
  cubeInfo2->normalMatrix = cubeInfo2->model.Inverse().Transpose();
  cubeInfo2->normalMatrix[3][0] = 0.0f;
  cubeInfo2->normalMatrix[3][1] = 0.0f;
  cubeInfo2->normalMatrix[3][2] = 0.0f;
  cubeInfo2->normalMatrix[3][3] = 1.0f;
  cubeInfo2->color = Vector4(0.8f, 0.8f, 1.0f, 1.0f);
  cubeInfo2->transparency = 0.4f;
  cubeInfo2->baseMetal = 0.5f;
  cubeInfo2->baseRough = 0.1f;

  Material cubeMaterial3;
  MeshDescriptor cubeMesh3;
  cubeMesh3.Initialize(&gRenderer());
  cubeMaterial3.SetAlbedo(albedo);
  ObjectBuffer* cubeInfo3 = cubeMesh3.ObjectData();
  cubeInfo3->model = Matrix4::Scale(Matrix4(), Vector3(0.1f, 0.1f, 0.1f)) * Matrix4::Translate(Matrix4::Identity(), light0Pos);
  cubeInfo3->normalMatrix = cubeInfo3->model.Inverse().Transpose();
  cubeInfo3->normalMatrix[3][0] = 0.0f;
  cubeInfo3->normalMatrix[3][1] = 0.0f;
  cubeInfo3->normalMatrix[3][2] = 0.0f;
  cubeInfo3->normalMatrix[3][3] = 1.0f;

#if PERFORMANCE_TEST
  std::array<MeshDescriptor, ObjectCount> MeshDescriptors;
  std::array<RenderObject*, ObjectCount> RenderObjects;
  std::array<Vector3, ObjectCount> Positions;
  
  for (size_t i = 0; i < MeshDescriptors.size(); ++i) {
    MeshDescriptors[i].Initialize(&gRenderer());
    r32 same = uni(r);
    Positions[i] = Vector3(same, uni(r), same);
    ObjectBuffer* buffer = MeshDescriptors[i].ObjectData();
    buffer->model = Matrix4::Scale(Matrix4::Identity(), Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Translate(Matrix4::Identity(), Positions[i]);
    buffer->normalMatrix = buffer->model.Inverse().Transpose();
    buffer->normalMatrix[3][0] = 0.0f;
    buffer->normalMatrix[3][1] = 0.0f;
    buffer->normalMatrix[3][2] = 0.0f;
    buffer->normalMatrix[3][3] = 1.0f;
    buffer->baseMetal = 0.5f;
    buffer->baseRough = 0.1f;
    buffer->color = Vector4(1.0f, 0.05f, 0.03f, 1.0f);
    RenderObjects[i] = gRenderer().CreateRenderObject();
    RenderObjects[i]->MaterialId = &cubeMaterial2;
    RenderObjects[i]->MeshDescriptorId = &MeshDescriptors[i];
    RenderObjects[i]->Initialize();
    RenderObjects[i]->PushBack(sphereMeshDat);
  }
#endif  

  RenderObject* obj1 = gRenderer().CreateRenderObject();
  RenderObject* obj2 = gRenderer().CreateRenderObject();
  RenderObject* obj3 = gRenderer().CreateRenderObject();

  obj1->MaterialId = &cubeMaterial;
  obj1->MeshDescriptorId = &cubeMesh;

  obj2->MaterialId = &cubeMaterial2;
  obj2->MeshDescriptorId = &cubeMesh2;
  obj2->Renderable = true;

  obj3->MaterialId = &cubeMaterial3;
  obj3->MeshDescriptorId = &cubeMesh3;

  obj1->Initialize();
  obj2->Initialize();
  obj3->Initialize();

  obj1->PushBack(cubeMeshDat);
  obj2->PushBack(sphereMeshDat);
  obj3->PushBack(cubeMeshDat);
  ///////////////////////////////////////////////////////////////////////////////////////

  /* Create the cmd list to send to the renderer. */
  // TODO(): This part should be something our engine worries about, not the user.
  // Plan: Create Game Objects from Engine, this will give engine handle to all objects
  // in game.
  // Transparent objects need to be rendered after opaque objects.
  CmdList& list = gEngine().RenderCommandList();
  list.Resize(3 
#if PERFORMANCE_TEST
    + RenderObjects.size()
#endif
  );
  list[0].target = obj3;
  list[1].target = obj1;
  list[2].target = obj2;

#if PERFORMANCE_TEST
  for (size_t i = 3; i < list.Size(); ++i) {
    list[i].target = RenderObjects[i - 3];
  }
#endif
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

#if PERFORMANCE_TEST
    for (size_t i = 0; i < MeshDescriptors.size(); ++i) {
      ObjectBuffer* buffer = MeshDescriptors[i].ObjectData();
      Vector3 Rev;
      r32 s = sinf((r32)Time::CurrentTime() * (1.0f - Absf(Positions[i].x / MaxNum)));
      r32 c = cosf((r32)Time::CurrentTime() * (1.0f - Absf(Positions[i].x / MaxNum)));

      Rev.x = c * Positions[i].x;
      Rev.z = s * Positions[i].z;
      Rev.y = Positions[i].y;
      buffer->model = Matrix4::Scale(Matrix4::Identity(), Vector3(1.0f, 1.0f, 1.0f)) * Matrix4::Translate(Matrix4::Identity(), Rev);
      buffer->normalMatrix = buffer->model.Inverse().Transpose();
      buffer->normalMatrix[3][0] = 0.0f;
      buffer->normalMatrix[3][1] = 0.0f;
      buffer->normalMatrix[3][2] = 0.0f;
      buffer->normalMatrix[3][3] = 1.0f;
    }
#endif
    // light cube transforming.
    light0Pos = Vector3(sinf((r32)Time::CurrentTime() * 1.0f) * -5.0f, 2.0f, 0.0f);
    lights->pointLights[0].position = Vector4(light0Pos, 1.0f);
    // Testing quat.
    Quaternion quat = Quaternion::AngleAxis(-Radians((r32)(Time::CurrentTime()) * 50.0f), Vector3(0.0f, 1.0f, 0.0f));
    cubeInfo3->model = Matrix4::Scale(Matrix4(), Vector3(0.1f, 0.1f, 0.1f)) * 
#if 0
      Matrix4::Rotate(Matrix4::Identity(), -Radians((r32)(Time::CurrentTime()) * 50.0f), Vector3(0.0f, 1.0f, 0.0f)) * 
#else
    quat.ToMatrix4() *
#endif
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
  gRenderer().FreeTexture2D(normal);
  gRenderer().FreeRenderObject(obj1);
  gRenderer().FreeRenderObject(obj2);
  gRenderer().FreeRenderObject(obj3);
  gRenderer().FreeMeshData(cubeMeshDat);
  gRenderer().FreeMeshData(sphereMeshDat);
  gRenderer().FreeLightDescriptor(lightDesc);
#if PERFORMANCE_TEST
  for (size_t i = 0; i < MeshDescriptors.size(); ++i) {
    MeshDescriptors[i].CleanUp();
    gRenderer().FreeRenderObject(RenderObjects[i]);
  }
#endif
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