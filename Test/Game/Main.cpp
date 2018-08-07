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


// Test scene that is used for setting up the game world.
class TestScene : public Scene {
  static const u32 kMaxCount = 1;
  static const u32 kNumberOfMonsters = 100;
public:

  // Used to set up the scene. Call before updating.
  void SetUp() override {
    cube = new CubeObject();
    lantern = new LanternObject();
    monster = new Monster();
    mainCam = new MainCamera();

    GetRoot()->AddChild(mainCam);
    mainCam->Start();

    for (u32 i = 0; i < kMaxCount; ++i) {
      helmets.push_back(new HelmetObject());
      GetRoot()->AddChild(helmets[i]);
      helmets[i]->Start();
    }

    std::random_device dev;
    std::mt19937 twist(dev());
    std::uniform_real_distribution<r32> uni(-10.0f, 10.0f);
    std::uniform_real_distribution<r32> above(0.0f, 20.0f);
    for (u32 i = 0; i < kNumberOfMonsters; ++i) {
      monsters[i] = new Monster();
      monsters[i]->Start();
      monsters[i]->SetPosition(Vector3(uni(twist), above(twist), uni(twist)));
      GetRoot()->AddChild(monsters[i]);
    }

    GetRoot()->AddChild(cube);
    GetRoot()->AddChild(lantern);
    GetRoot()->AddChild(monster);
    cube->Start();
    lantern->Start();
    monster->Start();

    // Set primary light.
    {
      Sky* pSky = GetSky();
      DirectionalLight* pPrimary = pSky->GetSunLight();
      pPrimary->_Ambient = Vector4(0.1f, 0.1f, 0.4f, 1.0f);
      pPrimary->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      pPrimary->_Direction = Vector3(1.0f, -0.5f, 0.0f).Normalize();
      pPrimary->_Enable = true;
      pPrimary->_Intensity = 5.0f;
    }
    
    // You are also allowed to configure hdr settings of the scene. This doesn't affect user parameters of
    // setting bloom on or off, or anything else...
    m_hdrSettings._bloomStrength = 1.0f;
  }

  // Start up function call, optional if no game object was called.
  void StartUp() override {
  }

  // Update function call. Called very loop iteration.
  void Update(r32 tick) override {
    mainCam->Update(tick);
    lantern->Update(tick);
    cube->Update(tick);
    for (size_t i = 0; i < kMaxCount; ++i) {
      helmets[i]->Update(tick);
    }

    if (Keyboard::KeyPressed(KEY_CODE_J)) {
      Camera::GetMain()->EnableInterleavedVideo(true);
    }
    if (Keyboard::KeyPressed(KEY_CODE_K)) {
      Camera::GetMain()->EnableInterleavedVideo(false);
    }
  }


  // Clean up function call.
  void CleanUp() override {
    // Clean up all game objects with done.
    for (u32 i = 0; i < kMaxCount; ++i) {
      helmets[i]->CleanUp();
      delete helmets[i];
    }

    for (u32 i = 0; i < kNumberOfMonsters; ++i) {
      monsters[i]->CleanUp();
      delete monsters[i];
    }

    cube->CleanUp();
    delete cube;

    lantern->CleanUp();
    delete lantern;

    monster->CleanUp();
    delete monster;

    mainCam->CleanUp();
    delete mainCam;
  }

private:
  std::vector<HelmetObject*>              helmets;
  std::array<Monster*, kNumberOfMonsters> monsters;
  CubeObject*                             cube;
  LanternObject*                          lantern;
  Monster*                                monster;
  MainCamera*                             mainCam;
};


int main(int c, char* argv[])
{
  Log::DisplayToConsole(true);
  Mouse::Enable(false);
  Mouse::Show(false);

  // Setting the renderer to vsync double buffering when starting up the engine,
  // Inputting gpu params is optional, and can pass nullptr if you prefer default.
  {
    GraphicsConfigParams params;
    params._Buffering = TRIPLE_BUFFER;
    params._EnableVsync = true;
    params._AA = AA_FXAA_2x;
    params._Shadows = SHADOWS_HIGH;
    params._TextureQuality = TEXTURE_QUALITY_ULTRA;
    params._EnableLocalReflections = true;
    params._EnableChromaticAberration = true;

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
    mesh->InitializeLod(boxVerts.size(), boxVerts.data(), MeshData::STATIC, 0, boxIndic.size(), boxIndic.data());
    mesh->SetMin(Cube::Min);
    mesh->SetMax(Cube::Max);
    mesh->UpdateAABB();
    MeshCache::Cache(RTEXT("NativeCube"), mesh);    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->GetMeshDataLod()->IndexData()->IndexCount();
    prim._pMat = Material::Default()->Native();
    mesh->PushPrimitive(prim);
  }

  {
    Mesh* mesh = new Mesh();
    i32 stckCnt = 32;
    i32 minus = 32 / 5;
    for (i32 lod = 0; lod < 5; ++lod) {
      auto sphereVerts = UVSphere::MeshInstance(1.0f, stckCnt, stckCnt);
      auto sphereInd = UVSphere::IndicesInstance(static_cast<u32>(sphereVerts.size()), stckCnt, stckCnt);
      mesh->InitializeLod(sphereVerts.size(), sphereVerts.data(), MeshData::STATIC, lod, sphereInd.size(), sphereInd.data());
      Primitive prim;
      prim._firstIndex = 0;
      prim._indexCount = mesh->GetMeshDataLod(lod)->IndexData()->IndexCount();
      prim._pMat = Material::Default()->Native();
      mesh->PushPrimitive(prim, lod);
      stckCnt -= minus;
    }
    MeshCache::Cache(RTEXT("NativeSphere"), mesh);
  }

  // Model Loading.
  ModelLoader::Load(RTEXT("Assets/DamagedHelmet/DamagedHelmet.gltf"));
  ModelLoader::Load(RTEXT("Assets/BoomBox/BoomBox.gltf"));
  ModelLoader::Load(RTEXT("Assets/Lantern/lantern.gltf"));
  ModelLoader::Load(RTEXT("Assets/Lantern2/Lantern.gltf"));
  ModelLoader::Load(RTEXT("Assets/SciFiHelmet/SciFiHelmet.gltf"));
  ModelLoader::LoadAnimatedModel(RTEXT("Assets/BrainStem/BrainStem.gltf"));
  ModelLoader::LoadAnimatedModel(RTEXT("Assets/Monster/Monster.gltf"));
  ModelLoader::LoadAnimatedModel(RTEXT("Assets/RiggedSimple.gltf"));
  ModelLoader::LoadAnimatedModel(RTEXT("Assets/busterDrone/busterDrone.gltf"));

  // Create and set up scene.

  // Create scene.
  TestScene scene;
  scene.SetUp();

  // Run engine, and push the scene to reference.
  gEngine().Run();
  gEngine().PushScene(&scene);

  // Optional startup call.
  scene.StartUp();

  ///////////////////////////////////////////////////////////////////////////////////

  // Game loop.
  while (gEngine().Running()) {
    Time::Update();
    gEngine().ProcessInput();
    scene.Update((r32)Time::FixTime);
    gEngine().Update();
  }

  // Once done using the scene, clean it up.
  scene.CleanUp();

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