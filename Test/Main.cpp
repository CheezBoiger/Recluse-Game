// Copyright (c) Recluse Project. All rights reserved.
#include "Game/Engine.hpp"
#include "Renderer/UserParams.hpp"

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
  // build the scene for the render. Should our cmd list be updated, you need to call
  // this function to update the scene.
  gRenderer().Build();
  r64 timeAccumulator = 0.0;


  // Game loop...
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

    // Syncronize engine modules, as they run on threads.
    gCore().Sync();
    gRenderer().Render();

    r64 fps = SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime);
    //printf("window width=%d\t\theight=%d\t\t\r", window.Width(), window.Height());
    printf("%f ms\t\t%d fps\t\t\r", timeAccumulator * 1000.0, u32(fps));

    Window::PollEvents();
  }
  
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