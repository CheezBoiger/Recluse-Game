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
void processInput()
{
  Camera* camera = Camera::getMain();;
  Window* window = gEngine().getWindow();

  // Test getGamma correction
  if (Keyboard::keyPressed(KEY_CODE_G)) { camera->setGamma(camera->getGamma() + (R32)(5.0 * Time::deltaTime)); }
  if (Keyboard::keyPressed(KEY_CODE_H)) { camera->setGamma(camera->getGamma() <= 0.0f ? 0.1f : camera->getGamma() - (R32)(5.0 * Time::deltaTime)); }
  // Test HDR Reinhard exposure.
  if (Keyboard::keyPressed(KEY_CODE_E)) { camera->setExposure(camera->getExposure() + (R32)(2.0 * Time::deltaTime)); }
  if (Keyboard::keyPressed(KEY_CODE_R)) { camera->setExposure(camera->getExposure() <= 0.0f ? 0.1f : camera->getExposure() - (R32)(2.0 * Time::deltaTime)); }

  if (Keyboard::keyPressed(KEY_CODE_0)) { camera->enableBloom(false); }
  if (Keyboard::keyPressed(KEY_CODE_1)) { camera->enableBloom(true); }

  // Camera projection changing.
  if (Keyboard::keyPressed(KEY_CODE_O)) { camera->setProjection(Camera::ORTHO); }
  if (Keyboard::keyPressed(KEY_CODE_P)) { camera->setProjection(Camera::PERSPECTIVE); }
  if (Keyboard::keyPressed(KEY_CODE_LEFT_ARROW)) { Time::scaleTime -= 0.5f * Time::deltaTime;  }
  if (Keyboard::keyPressed(KEY_CODE_RIGHT_ARROW)) { Time::scaleTime += 0.5f * Time::deltaTime; }

  // Window changing sets.
  if (Keyboard::keyPressed(KEY_CODE_M)) { window->setToFullScreen(); }
  if (Keyboard::keyPressed(KEY_CODE_N)) { window->setToWindowed(1200, 800); window->setToCenter(); window->show(); }

  if (Keyboard::keyPressed(KEY_CODE_ESCAPE)) { gEngine().signalStop(); }
}

#define SPHERE_SEGS 64
#define PERFORMANCE_TEST 1

// Simple Hello Cube example.
int main(int c, char* argv[])
{
  Log::displayToConsole(true);
  Mouse::show(false);

  gEngine().startUp(RTEXT("Recluse"), false, 1200, 800);
  // Optional: You may add an input callback to override the engine update.
  gEngine().setControlInput(processInput);

  {
    // In order to update the renderer during runtime, you can pass gpu configs to the
    // renderer directly.
    GraphicsConfigParams params;
    params._Buffering = DOUBLE_BUFFER;
    params._EnableVsync = Window::getRefreshRate() <= 60 ? true : false;
    params._AA = AA_None;
    params._Shadows = GRAPHICS_QUALITY_NONE;
    gRenderer().updateRendererConfigs(&params);
  }

  Window* window = gEngine().getWindow();    
  window->show();
  window->setToWindowed(Window::getFullscreenWidth(), Window::getFullscreenHeight(), true);

  printf(RTEXT("App directory: %s\n"), gFilesystem().CurrentAppDirectory());
  Log() << Window::getRefreshRate() << "\n";

  // Create a game object.
  // Create the scene.
  Scene scene;

  Mesh mesh;
  {
    auto boxVerts = Cube::meshInstance();
    auto boxIndic = Cube::indicesInstance();
    mesh.initialize(&gRenderer(), boxVerts.size(), boxVerts.data(), Mesh::STATIC, boxIndic.size(), boxIndic.data());
  }

  gEngine().run();
  gEngine().pushScene(&scene);

  while (gEngine().isRunning()) {
    Time::update();
    gEngine().update();
    gEngine().processInput();
  }

  mesh.cleanUp(&gRenderer());
  gEngine().cleanUp();
#if (_DEBUG)
  Log() << RTEXT("Engine modules cleaned up, press enter to continue...\n");
  std::cin.ignore();
#endif
  return 0;
}