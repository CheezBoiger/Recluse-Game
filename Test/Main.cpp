// Copyright (c) Recluse Project. All rights reserved.
#include "Game/Engine.hpp"
#include "DemoTextureLoad.hpp"

#include "Renderer/UserParams.hpp"

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
  window->Show();

  {
    GpuConfigParams params;
    params._Buffering = DOUBLE_BUFFER;
    params._EnableVsync = true;
    gRenderer().UpdateRendererConfigs(&params);
    window->SetToWindowed(Window::FullscreenWidth(), Window::FullscreenHeight(), true);
  }
  printf("App directory: %s\n", gFilesystem().CurrentAppDirectory());

  // TODO(): Old test was obsolete, will define a new test here soon.

  gEngine().CleanUp();
  return 0;
}