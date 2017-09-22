// Copyright (c) Recluse Project. All rights reserved.
#include "Core/Core.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Renderer/Renderer.hpp"
#include "Physics/Physics.hpp"
#include "Filesystem/Filesystem.hpp"
#include "Audio/Audio.hpp"
#include "UI/UI.hpp"

#include <stdio.h>

using namespace Recluse;

void KeyCallback(Window* window, i32 key, i32 scanCode, i32 action, i32 mods)
{
  static i32 keys[256];
  
  keys[key] = action; 
  
  if (keys[0x41] == WM_KEYDOWN) window->SetToFullScreen();
  if (keys[0x42] == WM_KEYDOWN) { window->SetToWindowed(1200, 800); window->Show(); }
  if (keys[0x43] == WM_KEYDOWN) { window->SetToWindowed(1200, 800, true); window->Show(); }
  if (keys[VK_ESCAPE] == WM_KEYDOWN) window->Close();
}


int main(int c, char* argv[])
{
  // NOTE(): Always start up the core first, before starting anything else up.
  gCore().StartUp();
  gFilesystem().StartUp();
  gRenderer().StartUp();
  gPhysics().StartUp();
  gAudio().StartUp();
  gUI().StartUp();

  Window::SetKeyboardCallback(KeyCallback);

  Window window;
  window.Create(RTEXT("私は猫が大好き"), 800, 600); 
  window.SetToWindowed(1200, 800);
  window.SetToCenter();
  window.Show();    

  gRenderer().Initialize(&window);
  r64 timeAccumulator = 0.0;

  // Game loop...
  while (!window.ShouldClose()) {
    Time::Update();

    timeAccumulator += Time::DeltaTime;

    // Render out the scene.
    gAudio().UpdateState();
    gUI().UpdateState();

    while (timeAccumulator > Time::FixTime) {
      // TODO(): Instead of sleeping, update the game state.
      gPhysics().UpdateState(Time::FixTime * Time::ScaleTime);
      Sleep(DWORD(Time::FixTime * 1000.0));
      timeAccumulator -= Time::DeltaTime;
    }

    // Syncronize engine modules, as they run on threads.
    gCore().Sync();
    gRenderer().Render();

    r64 fps = SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime);
    //printf("window width=%d\t\theight=%d\t\t\r", window.Width(), window.Height());
    printf("%f ms\t\t%d fps\t\t\r", timeAccumulator * 1000.0, u32(fps));

    Window::PollEvents();
  }
  
  gUI().ShutDown();
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