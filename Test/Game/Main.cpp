#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/TextureType.hpp"
#include "Game/Scene/ModelLoader.hpp"
#include "../DemoTextureLoad.hpp"
#include "Scripts/FlyViewCamera.hpp"
#include "Scripts/Helmet.hpp"
#include "Scripts/CubeObject.hpp"
#include "Scripts/Lantern.hpp"
#include "Core/Utility/Cpu.hpp"
#include "Renderer/LightProbe.hpp"

// Scripts.
#include <array>
#include <algorithm>
#include <random>
#include <cctype>

using namespace Recluse;


std::string GetOption(const std::string& line)
{
  size_t pos = line.find('=');
  if (pos == std::string::npos) return "";
  std::string option = line.substr(pos + 1);
  option.erase(std::remove_if(option.begin(), option.end(), [](u8 x) -> i32 { return std::isspace(x); }), option.end());
  std::transform(option.begin(), option.end(), option.begin(), std::tolower);
  return option;
}


b32 AvailableOption(const std::string& line, const tchar* option)
{
  size_t pos = line.find(option);
  if (pos != std::string::npos) return true;
  return false;
}


GraphicsConfigParams ReadGraphicsConfig(u32& w, u32& h)
{
  GraphicsConfigParams graphics = kDefaultGpuConfigs;
  FileHandle Buf;
  FilesystemResult result = gFilesystem().ReadFrom("Configs/RendererConfigs.recluse", &Buf);
  if (result == FilesystemResult_NotFound) {
    Log(rWarning) << "RendererConfigs not found! Setting default rendering configuration.\n";
    return graphics;
  }

  std::string line = "";
  for ( size_t i = 0; i < Buf.Sz; ++i ) {
    tchar ch = Buf.Buf[i];
    line.push_back(ch);
    if (ch == '\n') {
      std::cout << line;
      if (AvailableOption(line, "Buffering")) {
        std::string option = GetOption(line);
        if (option.compare("triple") == 0) {
          graphics._Buffering = TRIPLE_BUFFER;
        } else if (option.compare("single") == 0) {
          graphics._Buffering = SINGLE_BUFFER;
        } else {
          graphics._Buffering = DOUBLE_BUFFER;
        }
      }
      if (AvailableOption(line, "AntiAliasing")) {
        std::string option = GetOption(line);
        if (option.compare("fxaa") == 0) {
          graphics._AA = AA_FXAA_2x;
        } else if (option.compare("smaa2x") == 0) {
          graphics._AA = AA_SMAA_2x;
        } else {
          graphics._AA = AA_None;
        }
      }
      if (AvailableOption(line, "TextureQuality")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "ShadowQuality")) {
        std::string option = GetOption(line);
        if (option.compare("ultra") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_ULTRA;
        } else if (option.compare("high") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_HIGH;
        } else if (option.compare("medium") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_MEDIUM;
        } else if (option.compare("low") == 0) {
          graphics._Shadows = GRAPHICS_QUALITY_LOW;
        } else {
          graphics._Shadows = GRAPHICS_QUALITY_NONE;
        }
      }
      if (AvailableOption(line, "LightingQuality")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "ModelQuality")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "LevelOfDetail")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "RenderScale")) {
        std::string option = GetOption(line);
      }
      if (AvailableOption(line, "VSync")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableVsync = true;
        } else {
          graphics._EnableVsync = false;
        }
      }
      if (AvailableOption(line, "ChromaticAberration")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableChromaticAberration = true;
        } else {
          graphics._EnableChromaticAberration = false;
        }
      }
      if (AvailableOption(line, "Bloom")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableBloom = true;
        } else {
          graphics._EnableBloom = false;
        }
      }
      if (AvailableOption(line, "PostProcessing")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnablePostProcessing = true;
        } else {
          graphics._EnablePostProcessing = false;
        }
      }
      if (AvailableOption(line, "SoftShadows")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableSoftShadows = true;
        } else {
          graphics._EnableSoftShadows = false;
        }
      }
      if (AvailableOption(line, "Multithreading")) {
        std::string option = GetOption(line);
        if (option.compare("true") == 0) {
          graphics._EnableMultithreadedRendering = true;
        } else {
          graphics._EnableMultithreadedRendering = false;
        }
      }
      if (AvailableOption(line, "Resolution")) {
        std::string option = GetOption(line);
        if (option.compare("800x600") == 0) { graphics._Resolution = Resolution_800x600; w = 800; h = 600; }
        else if (option.compare("1200x800") == 0) { graphics._Resolution = Resolution_1200x800; w = 1200; h = 800; }
        else if (option.compare("1280x720") == 0) { graphics._Resolution = Resolution_1280x720; w = 1280; h = 720; }
        else if (option.compare("1440x900") == 0) { graphics._Resolution = Resolution_1440x900; w = 1400; h = 900; }
        else if (option.compare("1920x1080") == 0) { graphics._Resolution = Resolution_1920x1080; w = 1920; h = 1080; }
        else if (option.compare("1920x1440") == 0) { graphics._Resolution = Resolution_1920x1440; w = 1920; h = 1440; }
      }
      if (AvailableOption(line, "Window")) {
        std::string option = GetOption(line);
        if (option.compare("borderless") == 0) { graphics._WindowType = WindowType_Borderless; }
        else if (option.compare("fullscreen") == 0) { graphics._WindowType = WindowType_Fullscreen; }
        else if (option.compare("border") == 0) { graphics._WindowType = WindowType_Border; }
      }
      line.clear();
    }
  }
  return graphics;
}


// Test scene that is used for setting up the game world.
class TestScene : public Scene {
  static const u32 kMaxCount = 1;
  static const u32 kNumberOfMonsters = 0;
  TextureCube* cubemap1;
  TextureCube* cubemap0;
  Texture2D* brdfLUT;
public:

  // Used to set up the scene. Call before updating.
  void SetUp() override {
    cubemap1 = nullptr;
    cubemap0 = nullptr;
    brdfLUT = nullptr;
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
      pPrimary->_Direction = Vector3(0.08f, -0.5f, 0.08f).Normalize();
      pPrimary->_Enable = true;
      pPrimary->_Intensity = 5.0f;
    }
    
    // You are also allowed to configure hdr settings of the scene. This doesn't affect user parameters of
    // setting bloom on or off, or anything else...
    m_hdrSettings._bloomStrength = 1.0f;

#if 1
    cubemap0 = gRenderer().CreateTextureCube();
    cubemap1 = gRenderer().CreateTextureCube();
    brdfLUT = gRenderer().CreateTexture2D();
    cubemap0->Initialize(512);
    cubemap1->Initialize(512);
    brdfLUT->Initialize(RFORMAT_R8G8B8A8_UNORM, 512, 512);
    {
      Image img;
      img.Load("Assets/World/testcubemap.png");
      cubemap0->Update(img);
      img.CleanUp();
      img.Load("Assets/World/Probe1.png");
      cubemap1->Update(img);
      img.CleanUp();
      img.Load("Assets/World/brdf.png");
      brdfLUT->Update(img);
      img.CleanUp();
      gRenderer().SetSkyboxCubeMap(cubemap0);
      gRenderer().SetGlobalBRDFLUT(brdfLUT);
      gRenderer().UsePreRenderSkyboxMap(true);
    }
#endif
  }

  // Start up function call, optional if no game object was called.
  void StartUp() override {
  }

  // Update function call. Called very loop iteration.
  void Update(r32 tick) override {
    mainCam->Update(tick);
    lantern->Update(tick);
    cube->Update(tick);
    monster->Update(tick);
    for (size_t i = 0; i < kMaxCount; ++i) {
      helmets[i]->Update(tick);
    }

    if (Keyboard::KeyPressed(KEY_CODE_J)) {
      //Camera::GetMain()->EnableInterleavedVideo(true);
      gRenderer().SetSkyboxCubeMap(cubemap0);
      gRenderer().UsePreRenderSkyboxMap(true);
    }
    if (Keyboard::KeyPressed(KEY_CODE_K)) {
      //Camera::GetMain()->EnableInterleavedVideo(false);
      //gRenderer().SetSkyboxCubeMap(cubemap1);
      //gRenderer().UsePreRenderSkyboxMap(true);
      gRenderer().UsePreRenderSkyboxMap(false);
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

#if 1
    gRenderer().UsePreRenderSkyboxMap(false);
    gRenderer().FreeTextureCube(cubemap0);
    gRenderer().FreeTextureCube(cubemap1);
    gRenderer().FreeTexture2D(brdfLUT);
    cubemap0 = nullptr;
    cubemap1 = nullptr;
    brdfLUT = nullptr;
#endif
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
    u32 w = 800, h = 600;
    GraphicsConfigParams params = ReadGraphicsConfig(w, h);
    // Start up the engine and set the input controller.
    gEngine().StartUp(RTEXT("Recluse Test Game"), false, w, h, &params);
    gEngine().Run();
    Window* pWindow = gEngine().GetWindow();
    switch (params._WindowType) {
    case WindowType_Borderless:
    {
      pWindow->SetToWindowed(w, h, true);
      pWindow->SetToCenter();
    } break;
    case WindowType_Border:
    {
      pWindow->SetToWindowed(w, h);
      pWindow->SetToCenter();
    } break;
    case WindowType_Fullscreen:
    default:
      pWindow->SetToFullScreen();
    }
    pWindow->Show();
  }

  // One may also adjust the renderer settings during runtime as well, using the call
  // gRenderer().UpdateRendererConfigs().

  //Window* window = gEngine().GetWindow();
  // Need to show the window in order to see something.
  //window->Show();
  //window->SetToFullScreen();
  //window->SetToWindowed(Window::FullscreenWidth(), Window::FullscreenHeight(), true);

  ///////////////////////////////////////////////////////////////////////////////////
  // Everything within initialization will normally be handled by Managers, for now
  // we will be demonstrating manual initialization of various objects to render and
  // control something on the display.
  ///////////////////////////////////////////////////////////////////////////////////

  // manual loading of textures.
  LoadMaterials();

  // Mesh Loading.
  {
    Mesh* mesh = new Mesh();
    auto boxVerts = Cube::MeshInstance(); 
    auto boxIndic = Cube::IndicesInstance();
    mesh->Initialize(&gRenderer(), boxVerts.size(), boxVerts.data(), Mesh::STATIC, boxIndic.size(), boxIndic.data());
    mesh->SetMin(Cube::Min);
    mesh->SetMax(Cube::Max);
    mesh->UpdateAABB();
    MeshCache::Cache(RTEXT("NativeCube"), mesh);    
    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->GetMeshData()->IndexData()->IndexCount();
    prim._pMat = Material::Default();
    prim._localConfigs = 0;
    mesh->PushPrimitive(prim);
  }

  {
    Mesh* mesh = new Mesh();
    i32 stckCnt = 32;
    i32 minus = 32 / 5;
    //for (i32 lod = 0; lod < 5; ++lod) {
      auto sphereVerts = UVSphere::MeshInstance(1.0f, stckCnt, stckCnt);
      auto sphereInd = UVSphere::IndicesInstance(static_cast<u32>(sphereVerts.size()), stckCnt, stckCnt);
      mesh->Initialize(&gRenderer(), sphereVerts.size(), sphereVerts.data(), Mesh::STATIC, sphereInd.size(), sphereInd.data());
    //  stckCnt -= minus;
    //}
    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->GetMeshData()->IndexData()->IndexCount();
    prim._pMat = Material::Default();
    prim._localConfigs = 0;
    mesh->PushPrimitive(prim);
    MeshCache::Cache(RTEXT("NativeSphere"), mesh);
  }

  // Model Loading.
  ModelLoader::Load(RTEXT("Assets/DamagedHelmet/DamagedHelmet.gltf"));
  //ModelLoader::Load(RTEXT("Assets/BoomBox/BoomBox.gltf"));
  //ModelLoader::Load(RTEXT("Assets/Lantern/lantern.gltf"));
  ModelLoader::Load(RTEXT("Assets/Lantern2/Lantern.gltf"));
  //ModelLoader::Load(RTEXT("Assets/SciFiHelmet/SciFiHelmet.gltf"));
  ModelLoader::Load(RTEXT("Assets/BrainStem/BrainStem.gltf"));
  //ModelLoader::Load(RTEXT("Assets/Monster/Monster.gltf"));
  //ModelLoader::Load(RTEXT("Assets/CesiumMan.glb"));
  ModelLoader::Load(RTEXT("Assets/RiggedFigure.gltf"));
  ModelLoader::Load(RTEXT("Assets/RiggedSimple.gltf"));
  //ModelLoader::Load(RTEXT("Assets/busterDrone/busterDrone.gltf"));
 // ModelLoader::Load(RTEXT("Assets/BoxAnimated.glb"));
  ModelLoader::Load(RTEXT("Assets/sponza/Sponza.gltf"));
  ModelLoader::Load(RTEXT("Assets/WaterBottle.glb"));
  ModelLoader::Load(RTEXT("Assets/AnimatedMorphCube.gltf"));
  ModelLoader::Load(RTEXT("Assets/Wolf.glb"));
  ModelLoader::Load(RTEXT("Assets/AnimatedMorphSphere.glb"));
  //ModelLoader::Load(RTEXT("Assets/Wolf/Wolf.glb"));
  //ModelLoader::Load(RTEXT("Assets/Tree/tree.gltf"));

  // Create and set up scene.

  // Create scene.
  TestScene scene;
  scene.SetUp();

  // Run engine, and push the scene to reference.
  gEngine().PushScene(&scene);

  // Optional startup call.
  scene.StartUp();

  ///////////////////////////////////////////////////////////////////////////////////

  // Game loop.
#if 1
  while (gEngine().Running()) {
    Time::Update();
    gEngine().ProcessInput();
    scene.Update((r32)Time::DeltaTime);
    gEngine().Update();
  }
#else
  gEngine().SetEngineMode(EngineMode_Bake);
  // Testing enviroment probe map baking.
  std::array<Vector3, 5> positions = {
      Vector3(  0.0f,   -10.0f,     0.0f),
      Vector3(  0.0f,     0.0f,     0.0f),
      Vector3(  0.0f,   -14.0f,     0.0f),
      Vector3(  10.0f,    0.0f,     0.0f),
      Vector3(  0.0f,   -14.0f,     3.0f)
  };
  //gEngine().SetEnvProbeTargets(positions.data(), positions.size());
  gEngine().Update();
  Texture2D* g = gRenderer().GenerateBRDFLUT();
  g->Save("brdf.png");
  gRenderer().FreeTexture2D(g);
  gEngine().SignalStop();
  gEngine().Update();
#endif
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