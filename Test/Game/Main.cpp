#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"


#include "Game/Scene/ModelLoader.hpp"
#include "../DemoTextureLoad.hpp"
#include "Scripts/FlyViewCamera.hpp"
#include "Scripts/Helmet.hpp"
#include "Scripts/CubeObject.hpp"
#include "Scripts/Lantern.hpp"
#include "Core/Utility/Cpu.hpp"

// Scripts.
#include <array>
#include <algorithm>
#include <random>

using namespace Recluse;

/*
  Requirements for rendering something on screen:
    Material -> MaterialComponent.
    Mesh -> MeshComponent
    RendererComponent
  Updating materials or meshes are done through these components.
*/
int main(int c, char* argv[])
{
  Log::DisplayToConsole(true);
  Mouse::Enable(false);
  Mouse::Show(false);

  // Setting the renderer to vsync double buffering when starting up the engine,
  // Inputting gpu params is optional, and can pass nullptr if you prefer default.
  {
    GraphicsConfigParams params;
    params._Buffering = DOUBLE_BUFFER;
    params._EnableVsync = true;
    params._AA = AA_FXAA_2x;
    params._Shadows = SHADOWS_ULTRA;
    params._TextureQuality = TEXTURE_QUALITY_ULTRA;
    params._EnableLocalReflections = true;

    // Start up the engine and set the input controller.
    gEngine().StartUp(RTEXT("Recluse Test Game"), false, 1200, 800, &params);
  }

  // One may also adjust the renderer settings during runtime as well, using the call
  // gRenderer().UpdateRendererConfigs().

  Window* window = gEngine().GetWindow();
  // Need to show the window in order to see something.
  window->Show();
  //window->SetToFullScreen();
  window->SetToWindowed(Window::FullscreenWidth(), Window::FullscreenHeight(), true);

  ///////////////////////////////////////////////////////////////////////////////////
  // Everything within initialization will normally be handled by Managers, for now
  // we will be demonstrating manual initialization of various objects to render and
  // control something on the display.
  ///////////////////////////////////////////////////////////////////////////////////

  // manual loading of textures.
  LoadTextures();
  LoadMaterials();

  // Mesh Loading.
  {
    Mesh* mesh = new Mesh();
    auto boxVerts = Cube::MeshInstance(); 
    auto boxIndic = Cube::IndicesInstance();
    mesh->Initialize(boxVerts.size(), boxVerts.data(), MeshData::STATIC, boxIndic.size(), boxIndic.data());
    MeshCache::Cache(RTEXT("NativeCube"), mesh);
  }

  // Model Loading.
  ModelLoader::Load(RTEXT("Assets/DamagedHelmet/DamagedHelmet.gltf"));
  ModelLoader::Load(RTEXT("Assets/BoomBox/BoomBox.gltf"));
  ModelLoader::Load(RTEXT("Assets/Lantern/lantern.gltf"));
  ModelLoader::Load(RTEXT("Assets/Lantern2/Lantern.gltf"));
  ModelLoader::Load(RTEXT("Assets/SciFiHelmet/SciFiHelmet.gltf"));
  ModelLoader::LoadAnimatedModel(RTEXT("Assets/BrainStem/BrainStem.gltf"));
  ModelLoader::LoadAnimatedModel(RTEXT("Assets/Monster/Monster.gltf"));

  // Create and set up scene.
  MainCamera* mainCam = new MainCamera();
  // Create scene.
  Scene scene;
  scene.GetRoot()->AddChild(mainCam);
  
  std::vector<HelmetObject*> helmets;
  #define HELM_COUNT 1
  for (u32 i = 0; i < HELM_COUNT; ++i) {
    helmets.push_back(new HelmetObject());
    scene.GetRoot()->AddChild(helmets[i]);
  }

  CubeObject* cube = new CubeObject();
  LanternObject* lantern = new LanternObject();
  Monster* monster = new Monster();
  scene.GetRoot()->AddChild(cube);
  scene.GetRoot()->AddChild(lantern);
  scene.GetRoot()->AddChild(monster);

  // Add game objects into scene. This demonstrates parent-child transformation as well.

  // Set primary light.
  {
    Sky* pSky = scene.GetSky();
    DirectionalLight* pPrimary = pSky->GetSunLight();
    pPrimary->_Ambient = Vector4(0.1f, 0.1f, 0.4f, 1.0f);
    pPrimary->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector3(1.0f, -0.5f, 0.0f).Normalize();
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 5.0f;
  }

  // Second scene, to demonstrate the renderer's capabilities of transitioning multiple scenes.
  Scene scene2;
  scene2.GetRoot()->AddChild(mainCam);

  // Set primary light.
  {
    Sky* pSky = scene2.GetSky();
    DirectionalLight* pPrimary = pSky->GetSunLight();
    pPrimary->_Ambient = Vector4(0.3f, 0.3f, 0.66f, 1.0f);
    pPrimary->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector3(1.0f, -1.0f, 1.0f).Normalize();
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 2.0f;
  }


  // Run engine, and build the scene to render.
  gEngine().Run();
  gEngine().PushScene(&scene);
  gEngine().BuildScene();
  ///////////////////////////////////////////////////////////////////////////////////

  Log() << RTEXT("Timer Start: ") << Time::CurrentTime() << RTEXT(" s\n");
  // Game loop.
  while (gEngine().Running()) {
    Time::Update();
    gEngine().ProcessInput();
    gEngine().Update();
  }
  

  // Clean up all game objects with done.
  for (u32 i = 0; i < HELM_COUNT; ++i) {
    helmets[i]->CleanUp();
    delete helmets[i];
  }

  cube->CleanUp();
  delete cube;

  lantern->CleanUp();
  delete lantern;

  monster->CleanUp();
  delete monster;

  mainCam->CleanUp();
  delete mainCam;

  // Finish.
  AssetManager::CleanUpAssets();
  // Clean up engine
  gEngine().CleanUp();
#if (_DEBUG)
  Log() << RTEXT("Game is cleaned up. Press Enter to continue...\n");
  std::cin.ignore();
#endif
  return 0;
}