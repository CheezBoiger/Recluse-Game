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
  Updating materials or meshes requires you call RendererComponent::ReConfigure() to 
  update the object.
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

    // Start up the engine and set the input controller.
    gEngine().StartUp(RTEXT("Recluse Test Game"), false, 1200, 800, &params);
  }

  Window* window = gEngine().GetWindow();
  // Need to show the window in order to see something.
  window->Show();
  window->SetToWindowed(Window::FullscreenWidth(), Window::FullscreenHeight(), true);

  // Add game object in scene.
  LoadTextures();

  ///////////////////////////////////////////////////////////////////////////////////
  // Everything within initialization will normally be handled by Managers, for now
  // we will be demonstrating manual initialization of various objects to render and
  // control something on the display.
  ///////////////////////////////////////////////////////////////////////////////////

  {
    Mesh* mesh = new Mesh();
    u32 g = 32;
    auto boxVerts = Cube::MeshInstance();/* UVSphere::MeshInstance(1.0f, g, g);*/
    auto boxIndic = Cube::IndicesInstance();/*UVSphere::IndicesInstance(static_cast<u32>(boxVerts.size()), g, g);*/
    mesh->Initialize(boxVerts.size(), sizeof(StaticVertex), boxVerts.data(), true, boxIndic.size(), boxIndic.data());
    MeshCache::Cache("NativeCube", mesh);
  }

  ModelLoader::Load("Assets/DamagedHelmet/DamagedHelmet.gltf");
  ModelLoader::Load("Assets/BoomBox/BoomBox.gltf");
  ModelLoader::Load("Assets/Lantern/lantern.gltf");
  ModelLoader::Load("Assets/Lantern2/Lantern.gltf");

  {
    Material* material = new Material();
    material->Initialize();
    Texture2D* tex;
    TextureCache::Get("RustedAlbedo", &tex);

    material->SetAlbedo(tex);
    material->SetBaseColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    material->EnableAlbedo(true);
    material->SetRoughnessFactor(1.0f);
    material->SetMetallicFactor(1.0f);
    TextureCache::Get("RustedNormal", &tex);
    material->SetNormal(tex);
    material->EnableNormal(true);

    TextureCache::Get("RustedRough", &tex);
    material->SetRoughnessMetallic(tex);
    material->EnableRoughness(true);
    MaterialCache::Cache("RustySample", material);
  }

  MainCamera* mainCam = new MainCamera();
  // Create scene.
  Scene scene;
  scene.GetRoot()->AddChild(mainCam);
  
  std::vector<HelmetObject*> helmets;
  #define HELM_COUNT 100
  for (u32 i = 0; i < HELM_COUNT; ++i) {
    helmets.push_back(new HelmetObject());
    scene.GetRoot()->AddChild(helmets[i]);
  }

  CubeObject* cube = new CubeObject();
  LanternObject* lantern = new LanternObject();
  scene.GetRoot()->AddChild(cube);
  scene.GetRoot()->AddChild(lantern);

  // Add game objects into scene. This demonstrates parent-child transformation as well.

  // Set primary light.
  {
    DirectionalLight* pPrimary = scene.GetPrimaryLight();
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
    DirectionalLight* pPrimary = scene2.GetPrimaryLight();
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

  Log() << "Timer Start: " << Time::CurrentTime() << " s\n";
  // Game loop.
  while (gEngine().Running()) {
    Time::Update();
    gEngine().ProcessInput();

    // Test sun rendering. This is not mandatory for running the engine!
    //DirectionalLight* light = scene.GetPrimaryLight();
    //light->_Direction = Vector3(
    //  sinf(static_cast<r32>(Time::CurrentTime() * 0.1)), 
    //  cosf(static_cast<r32>(Time::CurrentTime() * 0.1))).Normalize();

    gEngine().Update();
    Log() << "FPS: " << SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime) << " fps\t\t\r";
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

  mainCam->CleanUp();
  delete mainCam;

  // Finish.
  ModelCache::CleanUpAll();
  MaterialCache::CleanUpAll();
  MeshCache::CleanUpAll();
  TextureCache::CleanUpAll();
  // Clean up engine
  gEngine().CleanUp();
#if (_DEBUG)
  Log() << "Game is cleaned up. Press Enter to continue...\n";
  std::cin.ignore();
#endif
  return 0;
}