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
#include "Renderer/MaterialDescriptor.hpp"
#include "Renderer/TextureType.hpp"
#include "Core/Utility/Image.hpp"

#include "DemoTextureLoad.hpp"

#include <stdio.h>
#include <array>
#include <random>
#include <thread>

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
  if (Keyboard::KeyReleased(KEY_CODE_SHIFT)) { FirstPersonCamera* fpsCamera = reinterpret_cast<FirstPersonCamera*>(camera); fpsCamera->SetSpeed(50.0f); }
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
  if (Keyboard::KeyPressed(KEY_CODE_V)) { gEngine().LightDesc()->EnablePrimaryShadow(true);/*noAlbedo2 = !noAlbedo2;*/ }
  if (Keyboard::KeyPressed(KEY_CODE_C)) { gEngine().LightDesc()->EnablePrimaryShadow(false);/*noAlbedo = !noAlbedo;*/ }

  // Camera projection changing.
  if (Keyboard::KeyPressed(KEY_CODE_O)) { camera->SetProjection(Camera::ORTHO); }
  if (Keyboard::KeyPressed(KEY_CODE_P)) { camera->SetProjection(Camera::PERSPECTIVE); }
  if (Keyboard::KeyPressed(KEY_CODE_LEFT_ARROW)) { Time::ScaleTime -= 0.5f * Time::DeltaTime;  }
  if (Keyboard::KeyPressed(KEY_CODE_RIGHT_ARROW)) { Time::ScaleTime += 0.5f * Time::DeltaTime; }

  // Window changing sets.
  if (Keyboard::KeyPressed(KEY_CODE_M)) { window->SetToFullScreen(); }
  if (Keyboard::KeyPressed(KEY_CODE_N)) { window->SetToWindowed(1200, 800); window->SetToCenter(); window->Show(); }
  if (Keyboard::KeyPressed(KEY_CODE_B)) { window->SetToWindowed(800, 600, true); window->SetToCenter(); window->Show(); }
  if (Keyboard::KeyPressed(KEY_CODE_ESCAPE)) { gEngine().SignalStop(); }
}

#define SPHERE_SEGS 64
#define PERFORMANCE_TEST 1

int main(int c, char* argv[])
{
  Log::DisplayToConsole(true);
  Mouse::Enable(false);
  Mouse::Show(false);

  gEngine().StartUp(RTEXT("Recluse"), false, 1200, 800);
  gEngine().SetControlInput(ProcessInput);
  Window* window = gEngine().GetWindow();    

  printf("App directory: %s\n", gFilesystem().CurrentAppDirectory());
  ///////////////////////////////////////////////////////////////////////////////////////
  // build the scene for the render. Should our cmd list be updated, you need to call
  // this function to update the scene. This is usually not hardcoded like this, as it 
  // is supposed to demonstrate how you can build a mesh and material outside the game 
  // loop.
  ///////////////////////////////////////////////////////////////////////////////////////
  Camera camera(Camera::PERSPECTIVE, Radians(55.0f), (r32)window->Width(), (r32)window->Height(), 0.0001f, 9000.0f, 
    Vector3(-4.0f, 4.0f, -4.0f), Vector3(0.0f, 0.0f, 1.0f));

  FirstPersonCamera fpsCamera(camera.FoV(), camera.PixelWidth(), 
    camera.PixelHeight(), camera.Near(), camera.Far(), Vector3(0.0f, 0.0f, -4.0f), Vector3(0.0f, 0.0f, -1.0f));
  fpsCamera.SetSpeed(50.0f);
  fpsCamera.EnableFrustumCull(true);

  Log(rVerbose) << "Global camera created, attaching to engine.\n";
  gEngine().SetCamera(&fpsCamera);
  Camera* gCamera = gEngine().GetCamera();

  // Get Light data from engine.
  LightBuffer* lights = gEngine().LightData();
  
  Vector3 light0Pos = Vector3(-0.0f, 10.0f, 1.0f);
  lights->_PrimaryLight._Direction = Vector4(0.7f, -0.3f, 0.0f, 1.0f);
  lights->_PrimaryLight._Intensity = 10.0f;
  lights->_PrimaryLight._Color = Vector4(1.0f, 1.0f, 0.7f, 1.0f);
  lights->_PrimaryLight._Ambient = Vector4(0.015f, 0.015f, 0.005f, 1.0f);
  lights->_PrimaryLight._Enable = true;

  lights->_DirectionalLights[0]._Enable = false;
  lights->_DirectionalLights[0]._Direction = Vector4(-1.0f, 1.0f, -1.0f, 1.0f);
  lights->_DirectionalLights[0]._Intensity = 1.0f;
  lights->_DirectionalLights[0]._Color = Vector4(1.0f, 0.8f, 0.4f, 1.0f);

  lights->_DirectionalLights[1]._Enable = false;
  lights->_DirectionalLights[1]._Direction = Vector4(-1.0f, 0.0f, -1.0f, 1.0f);
  lights->_DirectionalLights[1]._Intensity = 5.0f;
  lights->_DirectionalLights[1]._Color = Vector4(1.0f, 0.8f, 0.4f, 1.0f);

  lights->_DirectionalLights[2]._Enable = false;
  lights->_DirectionalLights[2]._Direction = Vector4(1.0f, 0.0f, -1.0f, 1.0f);
  lights->_DirectionalLights[2]._Intensity = 5.0f;
  lights->_DirectionalLights[2]._Color = Vector4(1.0f, 0.8f, 0.4f, 1.0f);

  lights->_PointLights[0]._Enable = false;
  lights->_PointLights[0]._Position = Vector4(light0Pos, 1.0f);
  lights->_PointLights[0]._Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  lights->_PointLights[0]._Range = 100.0f;
  lights->_PointLights[0]._Intensity = 1.0f;

  // Mimicking emissive texture on first box.
  lights->_PointLights[1]._Enable = false;
  lights->_PointLights[1]._Position = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  lights->_PointLights[1]._Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  lights->_PointLights[1]._Range = 5.0f;
  lights->_PointLights[1]._Intensity = 10.0f;

  LoadTextures();

  auto sphereData = UVSphere::MeshInstance(1.0f, SPHERE_SEGS, SPHERE_SEGS);
  auto sphereIndices = UVSphere::IndicesInstance((u32)sphereData.size(), SPHERE_SEGS, SPHERE_SEGS);
  MeshData* sphereMeshDat = gRenderer().CreateMeshData();
  sphereMeshDat->Initialize(sphereData.size(), sizeof(StaticVertex), sphereData.data(), true, sphereIndices.size(), sphereIndices.data());

  auto cubeData = Cube::MeshInstance();
  auto cubeIndices = Cube::IndicesInstance();
  MeshData* cubeMeshDat = gRenderer().CreateMeshData();
  cubeMeshDat->Initialize(cubeData.size(), sizeof(StaticVertex), cubeData.data(), true, cubeIndices.size(), cubeIndices.data());

#if PERFORMANCE_TEST
// Max: 3200
#define ObjectCount 40
  r32 maxNum = 100.0f;
  std::random_device gen;
  std::mt19937 r(gen());
  std::uniform_real_distribution<r32> uni(-maxNum, maxNum);
#endif
  Texture2D* tex = nullptr;
  MaterialDescriptor* cubeMaterial = gRenderer().CreateMaterialDescriptor();
  MeshDescriptor* cubeMesh = gRenderer().CreateStaticMeshDescriptor();
  cubeMesh->Initialize();
  cubeMaterial->Initialize();
  TextureCache::Get("BoxAlbedo", &tex);
  cubeMaterial->SetAlbedo(tex);
  TextureCache::Get("BoxNormal", &tex);
  cubeMaterial->SetNormal(tex);
  ObjectBuffer* cubeInfo = cubeMesh->ObjectData();
  MaterialBuffer* cubeMat = cubeMaterial->Data();
  cubeMat->_HasNormal = false;
  cubeMat->_BaseMetal = 0.0f;
  cubeMat->_BaseRough = 0.45f;
  cubeInfo->_Model = Matrix4::Rotate(Matrix4::Identity(), Radians(90.0f), Vector3(0.0f, 1.0f, 0.0f)) * Matrix4::Translate(Matrix4::Identity(), Vector3(0.0f, -70.0f, 1.0f));
  cubeInfo->_Model = Matrix4::Scale(cubeInfo->_Model, Vector3(50.0f, 50.0f, 50.0f));
  cubeInfo->_NormalMatrix = cubeInfo->_Model.Inverse().Transpose();
  cubeInfo->_NormalMatrix[3][0] = 0.0f;
  cubeInfo->_NormalMatrix[3][1] = 0.0f;
  cubeInfo->_NormalMatrix[3][2] = 0.0f;
  cubeInfo->_NormalMatrix[3][3] = 1.0f;

  MaterialDescriptor* cubeMaterial2 = gRenderer().CreateMaterialDescriptor();
  MeshDescriptor* cubeMesh2 = gRenderer().CreateStaticMeshDescriptor();
  cubeMaterial2->SetTransparent(false);
  cubeMesh2->Initialize();
  cubeMaterial2->Initialize();
  TextureCache::Get("RustedAlbedo", &tex);
  cubeMaterial2->SetAlbedo(tex);
  TextureCache::Get("RustedNormal", &tex);
  cubeMaterial2->SetNormal(tex);
  TextureCache::Get("RustedRough", &tex);
  cubeMaterial2->SetRoughness(tex);  
  TextureCache::Get("RustedMetal", &tex);
  cubeMaterial2->SetMetallic(tex);
  ObjectBuffer* cubeInfo2 = cubeMesh2->ObjectData();
  MaterialBuffer* cubeMat2 = cubeMaterial2->Data();
  cubeMat2->_HasNormal = true;
  cubeMat2->_HasRoughness = true;
  cubeMat2->_HasMetallic = true;
  cubeInfo2->_Model = Matrix4::Translate(Matrix4::Identity(), Vector3(10.0f, -14.9f, 0.0f));
  cubeInfo2->_Model = Matrix4::Rotate(cubeInfo2->_Model, Radians(45.0f), Vector3(0.0f, 1.0f, 0.0f));
  cubeInfo2->_Model = Matrix4::Scale(cubeInfo2->_Model, Vector3(5.0f, 5.0f, 5.0f));
  cubeInfo2->_NormalMatrix = cubeInfo2->_Model.Inverse().Transpose();
  cubeInfo2->_NormalMatrix[3][0] = 0.0f;
  cubeInfo2->_NormalMatrix[3][1] = 0.0f;
  cubeInfo2->_NormalMatrix[3][2] = 0.0f;
  cubeInfo2->_NormalMatrix[3][3] = 1.0f;
  cubeMat2->_Color = Vector4(0.8f, 0.8f, 1.0f, 1.0f);
  cubeMat2->_Opacity = 0.4f;
  cubeMat2->_BaseMetal = 0.0f;
  cubeMat2->_BaseRough = 0.1f;

  // Box using emissive map for light mimick.
  MaterialDescriptor* cubeMaterial3 = gRenderer().CreateMaterialDescriptor();
  MeshDescriptor* cubeMesh3 = gRenderer().CreateStaticMeshDescriptor();
  cubeMesh3->Initialize();
  cubeMaterial3->Initialize();
  TextureCache::Get("BoxAlbedo", &tex);
  cubeMaterial3->SetAlbedo(tex);
  TextureCache::Get("BoxEmissive", &tex);
  cubeMaterial3->SetEmissive(tex);
  ObjectBuffer* cubeInfo3 = cubeMesh3->ObjectData();
  MaterialBuffer* cubeMat3 = cubeMaterial3->Data();
  cubeInfo3->_Model = Matrix4::Scale(Matrix4(), Vector3(0.1f, 0.1f, 0.1f)) * Matrix4::Translate(Matrix4::Identity(), light0Pos);
  cubeInfo3->_NormalMatrix = cubeInfo3->_Model.Inverse().Transpose();
  cubeInfo3->_NormalMatrix[3][0] = 0.0f;
  cubeInfo3->_NormalMatrix[3][1] = 0.0f;
  cubeInfo3->_NormalMatrix[3][2] = 0.0f;
  cubeInfo3->_NormalMatrix[3][3] = 1.0f;
  cubeMat3->_HasEmissive = true;
  cubeMat3->_BaseMetal = 0.1f; 
  cubeMat3->_BaseRough = 0.9f;
  cubeMat3->_BaseEmissive = 3.15f;

#if PERFORMANCE_TEST
  // Multithreaded calculations at runtime.
  std::array<std::thread, 2>                workers;
  std::array<MeshDescriptor*, ObjectCount>  meshDescriptors;
  std::array<RenderObject*, ObjectCount>    renderObjects;
  std::array<Vector3, ObjectCount>          positions;
  size_t middle = meshDescriptors.size() / 2;
  MaterialDescriptor* material = gRenderer().CreateMaterialDescriptor();
  material->Initialize();
  MaterialBuffer* mat = material->Data();    
  mat->_BaseMetal = 0.5f;
  mat->_BaseRough = 0.1f;
  mat->_Color = Vector4(1.0f, 0.05f, 0.03f, 1.0f);

  for (size_t i = 0; i < meshDescriptors.size(); ++i) {
    meshDescriptors[i] = gRenderer().CreateStaticMeshDescriptor();
    meshDescriptors[i]->Initialize();
    r32 same = uni(r);
    positions[i] = Vector3(same, uni(r), same);
    ObjectBuffer* buffer = meshDescriptors[i]->ObjectData();
    buffer->_Model = Matrix4::Scale(Matrix4::Identity(), Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Translate(Matrix4::Identity(), positions[i]);
    buffer->_NormalMatrix = buffer->_Model.Inverse().Transpose();
    buffer->_NormalMatrix[3][0] = 0.0f;
    buffer->_NormalMatrix[3][1] = 0.0f;
    buffer->_NormalMatrix[3][2] = 0.0f;
    buffer->_NormalMatrix[3][3] = 1.0f;

    renderObjects[i] = gRenderer().CreateRenderObject();
    renderObjects[i]->MaterialId = material;
    renderObjects[i]->MeshDescriptorId = meshDescriptors[i];
    renderObjects[i]->Initialize();
    renderObjects[i]->PushBack(sphereMeshDat);
  }
#endif  

  RenderObject* obj1 = gRenderer().CreateRenderObject();
  RenderObject* obj2 = gRenderer().CreateRenderObject();
  RenderObject* obj3 = gRenderer().CreateRenderObject();

  obj1->MaterialId = cubeMaterial;
  obj1->MeshDescriptorId = cubeMesh;

  obj2->MaterialId = cubeMaterial2;
  obj2->MeshDescriptorId = cubeMesh2;
  obj2->Renderable = true;

  obj3->MaterialId = cubeMaterial3;
  obj3->MeshDescriptorId = cubeMesh3;

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
    + renderObjects.size()
#endif
  );
  list[0]._pTarget = obj3;
  list[1]._pTarget = obj1;
  list[2]._pTarget = obj2;

#if PERFORMANCE_TEST
  for (size_t i = 3; i < list.Size(); ++i) {
    list[i]._pTarget = renderObjects[i - 3];
  }
#endif
  gRenderer().Build();
  r64 timeAccumulator = 0.0;

  ///////////////////////////////////////////////////////////////////////////////////////
  // Game loop...
  ///////////////////////////////////////////////////////////////////////////////////////
  gEngine().Run();
  Log(rVerbose) << "Entering game loop...\n";

  while (gEngine().Running()) {
    Time::Update();
    
    r64 dt = Time::DeltaTime * Time::ScaleTime;
    r64 t = Time::CurrentTime() * Time::ScaleTime;
#if PERFORMANCE_TEST
    workers[0] = std::thread([&]() -> void {
      for (size_t i = 0; i < middle; ++i) {
        ObjectBuffer* buffer = meshDescriptors[i]->ObjectData();
        Vector3 rev;
        r32 s = sinf((r32)t * (1.0f - Absf(positions[i].x / maxNum)));
        r32 c = cosf((r32)t * (1.0f - Absf(positions[i].x / maxNum)));

        rev.x = c * positions[i].x;
        rev.z = s * positions[i].z;
        rev.y = positions[i].y;
        buffer->_Model = Matrix4::Scale(Matrix4::Identity(), Vector3(1.0f, 1.0f, 1.0f)) * Matrix4::Translate(Matrix4::Identity(), rev);
        buffer->_NormalMatrix = buffer->_Model.Inverse().Transpose();
        buffer->_NormalMatrix[3][0] = 0.0f;
        buffer->_NormalMatrix[3][1] = 0.0f;
        buffer->_NormalMatrix[3][2] = 0.0f;
        buffer->_NormalMatrix[3][3] = 1.0f;
      }
    });

    workers[1] = std::thread([&]() -> void {
      for (size_t i = middle; i < meshDescriptors.size(); ++i) {
        ObjectBuffer* buffer = meshDescriptors[i]->ObjectData();
        Vector3 rev;
        r32 s = sinf((r32)t * (1.0f - Absf(positions[i].x / maxNum)));
        r32 c = cosf((r32)t * (1.0f - Absf(positions[i].x / maxNum)));

        rev.x = c * positions[i].x;
        rev.z = s * positions[i].z;
        rev.y = positions[i].y;
        buffer->_Model = Matrix4::Scale(Matrix4::Identity(), Vector3(1.0f, 1.0f, 1.0f)) * Matrix4::Translate(Matrix4::Identity(), rev);
        buffer->_NormalMatrix = buffer->_Model.Inverse().Transpose();
        buffer->_NormalMatrix[3][0] = 0.0f;
        buffer->_NormalMatrix[3][1] = 0.0f;
        buffer->_NormalMatrix[3][2] = 0.0f;
        buffer->_NormalMatrix[3][3] = 1.0f;
      }
    });

    workers[0].join();
    workers[1].join();
#endif
    // light cube transforming.
    //light0Pos = Vector3(sinf((r32)t * 1.0f) * -5.0f, 2.0f, 0.0f);
    light0Pos += Vector3(0.0f, -1.0f, 0.0f) * static_cast<r32>(dt);
    lights->_PointLights[0]._Position = Vector4(light0Pos, 1.0f);
    // Testing quat.
    Quaternion quat = Quaternion::AngleAxis(-Radians((r32)(t) * 50.0f), Vector3(0.0f, 1.0f, 0.0f));
    cubeInfo3->_Model = Matrix4::Scale(Matrix4(), Vector3(1.5f, 1.5f, 1.5f)) * 
#if 0
      Matrix4::Rotate(Matrix4::Identity(), -Radians((r32)(Time::CurrentTime()) * 50.0f), Vector3(0.0f, 1.0f, 0.0f)) * 
#else
    quat.ToMatrix4() *
#endif
    Matrix4::Translate(Matrix4::Identity(), light0Pos);
    cubeInfo3->_NormalMatrix = cubeInfo3->_Model.Inverse().Transpose();
    cubeInfo3->_NormalMatrix[3][0] = 0.0f;
    cubeInfo3->_NormalMatrix[3][1] = 0.0f;
    cubeInfo3->_NormalMatrix[3][2] = 0.0f;
    cubeInfo3->_NormalMatrix[3][3] = 1.0f;

    if (noAlbedo2) { cubeMaterial2->Data()->_HasAlbedo = false; } else { cubeMaterial2->Data()->_HasAlbedo = true; }
    if (noAlbedo) { cubeMaterial->Data()->_HasAlbedo = false; } else { cubeMaterial->Data()->_HasAlbedo = true; }

    // //////////
    // End updates.
    // //////////
    //Log(rDebug) << gCamera->Front() << "\n";

    // Syncronize engine modules, as they run on threads.
    gEngine().Update();

    r64 fps = SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime);
    //printf("window width=%d\t\theight=%d\t\t\r", window.Width(), window.Height());
    printf("%f ms\t\t%d fps\t\t\t\r", timeAccumulator * 1000.0, u32(fps));

    gEngine().ProcessInput();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // Free up resources that were allocated.
  ///////////////////////////////////////////////////////////////////////////////////////
  TextureCleanUp();

  gRenderer().FreeRenderObject(obj1);
  gRenderer().FreeRenderObject(obj2);
  gRenderer().FreeRenderObject(obj3);
  gRenderer().FreeMeshData(cubeMeshDat);
  gRenderer().FreeMeshData(sphereMeshDat);
#if PERFORMANCE_TEST
  for (size_t i = 0; i < meshDescriptors.size(); ++i) {
    gRenderer().FreeMeshDescriptor(meshDescriptors[i]);
    gRenderer().FreeRenderObject(renderObjects[i]);
  }
  gRenderer().FreeMaterialDescriptor(material);
#endif
  gRenderer().FreeMeshDescriptor(cubeMesh);
  gRenderer().FreeMeshDescriptor(cubeMesh2);
  gRenderer().FreeMeshDescriptor(cubeMesh3);
  gRenderer().FreeMaterialDescriptor(cubeMaterial);
  gRenderer().FreeMaterialDescriptor(cubeMaterial2);
  gRenderer().FreeMaterialDescriptor(cubeMaterial3);
  ///////////////////////////////////////////////////////////////////////////////////////  
  gEngine().CleanUp();
#if (_DEBUG)
  printf("\nEngine modules cleaned up, Press Enter to continue...\n");
  getchar();
#endif
  return EXIT_SUCCESS;
}