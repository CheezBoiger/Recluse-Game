// Copyright (c) Recluse Project. All rights reserved.
#include "Game/Engine.hpp"
#include "DemoTextureLoad.hpp"

#include "Renderer/UserParams.hpp"
#include "Game/MeshComponent.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/MaterialComponent.hpp"

using namespace Recluse;

bool noAlbedo2 = false;
bool noAlbedo = false;

// TODO(): This just demonstrates key input. Normally you would use it for,
// say, moving a character and such.
void ProcessInput()
{
  Camera* camera = gEngine().GetCamera();
  Window* window = gEngine().GetWindow();

  if (Keyboard::KeyPressed(KEY_CODE_SHIFT)) { FirstPersonCamera* fpsCamera = reinterpret_cast<FirstPersonCamera*>(camera); fpsCamera->SetSpeed(500.0f); }
  if (Keyboard::KeyReleased(KEY_CODE_SHIFT)) { FirstPersonCamera* fpsCamera = reinterpret_cast<FirstPersonCamera*>(camera); fpsCamera->SetSpeed(50.0f); }
  if (Keyboard::KeyPressed(KEY_CODE_W)) { camera->Move(Camera::FORWARD, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_S)) { camera->Move(Camera::BACK, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_D)) { camera->Move(Camera::RIGHT, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_A)) { camera->Move(Camera::LEFT, Time::DeltaTime); }

  // Test Gamma correction
  if (Keyboard::KeyPressed(KEY_CODE_G)) { camera->SetGamma(camera->Gamma() + (r32)(5.0 * Time::DeltaTime)); }
  if (Keyboard::KeyPressed(KEY_CODE_H)) { camera->SetGamma(camera->Gamma() <= 0.0f ? 0.1f : camera->Gamma() - (r32)(5.0 * Time::DeltaTime)); }
  // Test HDR Reinhard exposure.
  if (Keyboard::KeyPressed(KEY_CODE_E)) { camera->SetExposure(camera->Exposure() + (r32)(2.0 * Time::DeltaTime)); }
  if (Keyboard::KeyPressed(KEY_CODE_R)) { camera->SetExposure(camera->Exposure() <= 0.0f ? 0.1f : camera->Exposure() - (r32)(2.0 * Time::DeltaTime)); }

  if (Keyboard::KeyPressed(KEY_CODE_0)) { camera->EnableBloom(false); }
  if (Keyboard::KeyPressed(KEY_CODE_1)) { camera->EnableBloom(true); }

  // Camera projection changing.
  if (Keyboard::KeyPressed(KEY_CODE_O)) { camera->SetProjection(Camera::ORTHO); }
  if (Keyboard::KeyPressed(KEY_CODE_P)) { camera->SetProjection(Camera::PERSPECTIVE); }
  if (Keyboard::KeyPressed(KEY_CODE_LEFT_ARROW)) { Time::ScaleTime -= 0.5f * Time::DeltaTime;  }
  if (Keyboard::KeyPressed(KEY_CODE_RIGHT_ARROW)) { Time::ScaleTime += 0.5f * Time::DeltaTime; }

  // Window changing sets.
  if (Keyboard::KeyPressed(KEY_CODE_M)) { window->SetToFullScreen(); }
  if (Keyboard::KeyPressed(KEY_CODE_N)) { window->SetToWindowed(1200, 800); window->SetToCenter(); window->Show(); }

  if (Keyboard::KeyPressed(KEY_CODE_ESCAPE)) { gEngine().SignalStop(); }
}

#define SPHERE_SEGS 64
#define PERFORMANCE_TEST 1

// Simple Hello Cube example.
int main(int c, char* argv[])
{
  Log::DisplayToConsole(true);
  Mouse::Enable(false);
  Mouse::Show(false);

  gEngine().StartUp(RTEXT("Recluse"), false, 1200, 800);
  gEngine().SetControlInput(ProcessInput);

  {
    // In order to update the renderer during runtime, you can pass gpu configs to the
    // renderer directly.
    GpuConfigParams params;
    params._Buffering = DOUBLE_BUFFER;
    params._EnableVsync = true;
    params._AA = AA_None;
    params._Shadows = SHADOWS_NONE;
    gRenderer().UpdateRendererConfigs(&params);
  }

  Window* window = gEngine().GetWindow();    
  window->Show();
  window->SetToWindowed(Window::FullscreenWidth(), Window::FullscreenHeight(), true);

  printf("App directory: %s\n", gFilesystem().CurrentAppDirectory());

  Camera camera(Camera::PERSPECTIVE, Radians(45.0f), 
    static_cast<r32>(window->Width()), 
    static_cast<r32>(window->Height()), 0.001f, 1000.0f, Vector3(5.0f, 5.0f, -5.0f), Vector3(0.0f, 0.0f, 0.0f));
  gEngine().SetCamera(&camera);

  // Create a game object.
  GameObject* obj = GameObject::Instantiate();
  // Create the scene.
  Scene scene;
  scene.GetRoot()->AddChild(obj);
  // Set primary light.
  {
    DirectionalLight* pPrimary = scene.GetPrimaryLight();
    pPrimary->_Ambient = Vector4(0.1f, 0.1f, 0.1f, 1.0f);
    pPrimary->_Color = Vector4(0.7f, 0.7f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector4(1.0f, -1.0f, 1.0f);
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 5.0f;
  }

  Mesh mesh;
  {
    auto boxVerts = Cube::MeshInstance();
    auto boxIndic = Cube::IndicesInstance();
    mesh.Initialize(boxVerts.size(), sizeof(StaticVertex), boxVerts.data(), true, boxIndic.size(), boxIndic.data());
  }

  obj->AddComponent<Transform>();
  obj->AddComponent<MeshComponent>();
  MeshComponent* mc = obj->GetComponent<MeshComponent>();
  mc->SetMeshRef(&mesh);
  obj->AddComponent<RendererComponent>();

  gEngine().Run();
  gEngine().PushScene(&scene);
  gEngine().BuildScene();

  while (gEngine().Running()) {
    Time::Update();
    gEngine().Update();
    gEngine().ProcessInput();
  }

  mesh.CleanUp();
  GameObject::DestroyAll();
  gEngine().CleanUp();
#if (_DEBUG)
  Log() << "Engine modules cleaned up, press enter to continue...\n";
  std::cin.ignore();
#endif
  return 0;
}