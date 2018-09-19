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
// say, moving a character and such. Be advised, THIS IS ONLY USED for overriding 
// engine input. It would be wise to create you own game object that controls the
// camera instead.
void ProcessInput()
{
  Camera* camera = Camera::GetMain();;
  Window* window = gEngine().GetWindow();

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
  // Optional: You may add an input callback to override the engine update.
  gEngine().SetControlInput(ProcessInput);

  {
    // In order to update the renderer during runtime, you can pass gpu configs to the
    // renderer directly.
    GraphicsConfigParams params;
    params._Buffering = DOUBLE_BUFFER;
    params._EnableVsync = true;
    params._AA = AA_None;
    params._Shadows = GRAPHICS_QUALITY_NONE;
    gRenderer().UpdateRendererConfigs(&params);
  }

  Window* window = gEngine().GetWindow();    
  window->Show();
  window->SetToWindowed(Window::FullscreenWidth(), Window::FullscreenHeight(), true);

  printf(RTEXT("App directory: %s\n"), gFilesystem().CurrentAppDirectory());

  // Create a game object.
  // Create the scene.
  Scene scene;

  Mesh mesh;
  {
    auto boxVerts = Cube::MeshInstance();
    auto boxIndic = Cube::IndicesInstance();
    mesh.Initialize(&gRenderer(), boxVerts.size(), boxVerts.data(), Mesh::STATIC, boxIndic.size(), boxIndic.data());
  }

  gEngine().Run();
  gEngine().PushScene(&scene);

  while (gEngine().Running()) {
    Time::Update();
    gEngine().Update();
    gEngine().ProcessInput();
  }

  mesh.CleanUp(&gRenderer());
  gEngine().CleanUp();
#if (_DEBUG)
  Log() << RTEXT("Engine modules cleaned up, press enter to continue...\n");
  std::cin.ignore();
#endif
  return 0;
}