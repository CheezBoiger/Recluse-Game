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
        else if (option.compare("1920x1200") == 0) { graphics._Resolution = Resolution_1920x1200; w = 1920; h = 1200; }
        else if (option.compare("2048x1440") == 0) { graphics._Resolution = Resolution_2048x1440; w = 2048; h = 1440; }
        else if (option.compare("3840x2160") == 0) { graphics._Resolution = Resolution_3840x2160; w = 3840; h = 2160; }
        else if (option.compare("7680x4320") == 0) { graphics._Resolution = Resolution_7680x4320; w = 7680; h = 4320; }
        else if (option.compare("unknown") == 0) { }
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
  static const u32 kNumberOfMonsters = 1;
  TextureCube* cubemap1;
  TextureCube* cubemap0;
  Texture2D* brdfLUT;
  CubeObject* acube;
public:

  // Used to set up the scene. Call before updating.
  void setUp() override {
    cubemap1 = nullptr;
    cubemap0 = nullptr;
    brdfLUT = nullptr;
    cube = new CubeObject(true);
    acube = new CubeObject(false);
    lantern = new LanternObject();
    monster = new Monster();
    mainCam = new MainCamera();
    mover = new Mover();
    mover->pMainCam = mainCam;

    getRoot()->addChild(mainCam);
    getRoot()->addChild(acube);
    getRoot()->addChild(mover);
    mainCam->start();
    mover->start();

    for (u32 i = 0; i < kMaxCount; ++i) {
      helmets.push_back(new HelmetObject());
      getRoot()->addChild(helmets[i]);
      helmets[i]->start();
    }

    std::random_device dev;
    std::mt19937 twist(dev());
    std::uniform_real_distribution<r32> uni(-10.0f, 10.0f);
    std::uniform_real_distribution<r32> above(0.0f, 20.0f);
    for (u32 i = 0; i < kNumberOfMonsters; ++i) {
      monsters[i] = new Monster();
      monsters[i]->start();
      monsters[i]->setPosition(Vector3(uni(twist), above(twist), uni(twist)));
      getRoot()->addChild(monsters[i]);
    }

    getRoot()->addChild(cube);
    getRoot()->addChild(lantern);
    getRoot()->addChild(monster);
    cube->start();  //cube->getTransform()->_scale = Vector3(100.0f, 100.0f, 100.0f);
    //acube->Start(); acube->getTransform()->_position = Vector3(0.0f, -300.0f, 0.0f);
    //cube->getTransform()->_scale = Vector3(50.0f, 50.0f, 50.0f);
    lantern->start();
    monster->start();

    // Set primary light.
    {
      Sky* pSky = getSky();
      DirectionalLight* pPrimary = pSky->getSunLight();
      pPrimary->_Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
      pPrimary->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      pPrimary->_Direction = Vector3(0.08f, -0.5f, 0.08f).normalize();
      pPrimary->_Enable = true;
      pPrimary->_Intensity = 5.0f;
      //pSky->SetSkyColor(Vector3(0.0f, 0.0f, 0.0f));
      //pSky->SetSkyIntensity(50.0f);
    }
    
    // You are also allowed to configure hdr settings of the scene. This doesn't affect user parameters of
    // setting bloom on or off, or anything else...
    m_hdrSettings._bloomStrength = 1.0f;

#if 1
    cubemap0 = gRenderer().createTextureCube();
    cubemap1 = gRenderer().createTextureCube();
    brdfLUT = gRenderer().createTexture2D();
    cubemap0->initialize(512);
    cubemap1->initialize(512);
    brdfLUT->initialize(RFORMAT_R8G8B8A8_UNORM, 512, 512);
    {
      Image img;
      img.load("Assets/World/testcubemap.png");
      cubemap0->update(img);
      img.cleanUp();
      img.load("Assets/World/Probe0.png");
      cubemap1->update(img);
      img.cleanUp();
      img.load("Assets/World/brdf.png");
      brdfLUT->update(img);
      img.cleanUp();
      lightprobe.generateSHCoefficients(gRenderer().getRHI(), cubemap0);
      lightprobe._bias = 0.55f;
      otherLightProbe.generateSHCoefficients(gRenderer().getRHI(), cubemap1);
      otherLightProbe._bias = 1.0f;
      gRenderer().setGlobalLightProbe(&lightprobe);
      gRenderer().setSkyboxCubeMap(cubemap0);
      gRenderer().setGlobalBRDFLUT(brdfLUT);
      gRenderer().usePreRenderSkyboxMap(true);
    }
#endif
  }

  // Start up function call, optional if no game object was called.
  void startUp() override {
  }

  // Update function call. Called very loop iteration.
  void update(r32 tick) override {
    mover->update(tick);
    mainCam->update(tick);
    lantern->update(tick);
    cube->update(tick);
    monster->update(tick);
    for (size_t i = 0; i < kMaxCount; ++i) {
      helmets[i]->update(tick);
    }

    for (size_t i = 0; i < kNumberOfMonsters; ++i) {
      monsters[i]->update(tick);
    }

    if (Keyboard::keyPressed(KEY_CODE_J)) {
      //Camera::getMain()->enableInterleavedVideo(true);
      gRenderer().setSkyboxCubeMap(cubemap0);
      gRenderer().setGlobalLightProbe(&lightprobe);
      gRenderer().usePreRenderSkyboxMap(true);
    }
    if (Keyboard::keyPressed(KEY_CODE_K)) {
      //Camera::getMain()->enableInterleavedVideo(false);
      //gRenderer().SetSkyboxCubeMap(cubemap1);
      //gRenderer().usePreRenderSkyboxMap(true);
      //gRenderer().setGlobalLightProbe(&otherLightProbe);
      gRenderer().setGlobalLightProbe(&otherLightProbe);
      gRenderer().usePreRenderSkyboxMap(false);
    }
  }


  // Clean up function call.
  void cleanUp() override {
    // Clean up all game objects with done.
    for (u32 i = 0; i < kMaxCount; ++i) {
      helmets[i]->cleanUp();
      delete helmets[i];
    }

    for (u32 i = 0; i < kNumberOfMonsters; ++i) {
      monsters[i]->cleanUp();
      delete monsters[i];
    }

    cube->cleanUp();
    delete cube;

    lantern->cleanUp();
    delete lantern;

    monster->cleanUp();
    delete monster;

    mainCam->cleanUp();
    delete mainCam;

    acube->cleanUp();
    delete acube;

#if 1
    gRenderer().usePreRenderSkyboxMap(false);
    gRenderer().freeTextureCube(cubemap0);
    gRenderer().freeTextureCube(cubemap1);
    gRenderer().freeTexture2D(brdfLUT);
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
  LightProbe                              lightprobe;
  LightProbe                              otherLightProbe;
  Monster*                                monster;
  MainCamera*                             mainCam;
  Mover*                                  mover;
};


int main(int c, char* argv[])
{
  Log::displayToConsole(true);
  Mouse::setEnable(false);
  Mouse::show(false);

  // Setting the renderer to vsync double buffering when starting up the engine,
  // Inputting gpu params is optional, and can pass nullptr if you prefer default.
  {
    u32 w = 800, h = 600;
    GraphicsConfigParams params = ReadGraphicsConfig(w, h);
    // Start up the engine and set the input controller.
    gEngine().startUp(RTEXT("Recluse Test Game"), false, w, h, &params);
    gEngine().run();
    Window* pWindow = gEngine().getWindow();
    switch (params._WindowType) {
    case WindowType_Borderless:
    {
      pWindow->setToWindowed(w, h, true);
      pWindow->setToCenter();
    } break;
    case WindowType_Border:
    {
      pWindow->setToWindowed(w, h);
      pWindow->setToCenter();
    } break;
    case WindowType_Fullscreen:
    default:
      pWindow->setToFullScreen();
    }
    pWindow->show();
  }

  // One may also adjust the renderer settings during runtime as well, using the call
  // gRenderer().updateRendererConfigs().

  //Window* window = gEngine().GetWindow();
  // Need to show the window in order to see something.
  //window->show();
  //window->setToFullScreen();
  //window->setToWindowed(Window::getFullscreenWidth(), Window::getFullscreenHeight(), true);

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
    auto boxVerts = Cube::meshInstance(); 
    auto boxIndic = Cube::indicesInstance();
    mesh->initialize(&gRenderer(), boxVerts.size(), boxVerts.data(), Mesh::STATIC, boxIndic.size(), boxIndic.data());
    mesh->setMin(Cube::minimum);
    mesh->setMax(Cube::maximum);
    mesh->updateAABB();
    MeshCache::cache(RTEXT("NativeCube"), mesh);    
    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->getMeshData()->getIndexData()->IndexCount();
    prim._pMat = Material::getDefault();
    prim._localConfigs = 0;
    mesh->pushPrimitive(prim);
  }

  {
    Mesh* mesh = new Mesh();
    i32 stckCnt = 32;
    i32 minus = 32 / 5;
    //for (i32 lod = 0; lod < 5; ++lod) {
      auto sphereVerts = UVSphere::meshInstance(1.0f, stckCnt, stckCnt);
      auto sphereInd = UVSphere::indicesInstance(static_cast<u32>(sphereVerts.size()), stckCnt, stckCnt);
      mesh->initialize(&gRenderer(), sphereVerts.size(), sphereVerts.data(), Mesh::STATIC, sphereInd.size(), sphereInd.data());
    //  stckCnt -= minus;
    //}
    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->getMeshData()->getIndexData()->IndexCount();
    prim._pMat = Material::getDefault();
    prim._localConfigs = 0;
    mesh->pushPrimitive(prim);
    MeshCache::cache(RTEXT("NativeSphere"), mesh);
  }

  // Model Loading.
  ModelLoader::load(RTEXT("Assets/DamagedHelmet/DamagedHelmet.gltf"));
  //ModelLoader::load(RTEXT("Assets/BoomBox/BoomBox.gltf"));
  //ModelLoader::load(RTEXT("Assets/Lantern/lantern.gltf"));
  ModelLoader::load(RTEXT("Assets/Lantern2/Lantern.gltf"));
  //ModelLoader::load(RTEXT("Assets/SciFiHelmet/SciFiHelmet.gltf"));
  ModelLoader::load(RTEXT("Assets/BrainStem/BrainStem.gltf"));
  //ModelLoader::load(RTEXT("Assets/Monster/Monster.gltf"));
  //ModelLoader::load(RTEXT("Assets/CesiumMan.glb"));
  //ModelLoader::load(RTEXT("Assets/RiggedFigure.gltf"));
  //ModelLoader::load(RTEXT("Assets/RiggedSimple.gltf"));
  //ModelLoader::load(RTEXT("Assets/busterDrone/busterDrone.gltf"));
 // ModelLoader::load(RTEXT("Assets/BoxAnimated.glb"));
  ModelLoader::load(RTEXT("Assets/sponza/Sponza.gltf"));
  //ModelLoader::load(RTEXT("Assets/busterDrone/busterDrone.gltf"));
  //ModelLoader::load(RTEXT("Assets/ux3d-industrial-robot/source/robot.glb"));
  //ModelLoader::load(RTEXT("Assets/WaterBottle.glb"));
  //ModelLoader::load(RTEXT("Assets/AnimatedMorphCube.gltf"));
  //ModelLoader::load(RTEXT("Assets/Wolf.glb"));
  //ModelLoader::load(RTEXT("Assets/AnimatedMorphSphere.glb"));
  //ModelLoader::load(RTEXT("Assets/Wolf/Wolf.glb"));
  //ModelLoader::load(RTEXT("Assets/Tree/tree.gltf"));

  // Create and set up scene.

  // Create scene.
  TestScene scene;
  scene.setUp();

  // Run engine, and push the scene to reference.
  gEngine().pushScene(&scene);

  // Optional startup call.
  scene.startUp();

  ///////////////////////////////////////////////////////////////////////////////////

  // Game loop.
#if 1
  while (gEngine().isRunning()) {
    Time::update();
    gEngine().processInput();
    scene.update((r32)Time::deltaTime);
    gEngine().update();
  }
#else
  gEngine().setEngineMode(EngineMode_Bake);
  // Testing enviroment probe map baking.
  std::array<Vector3, 5> positions = {
      Vector3(  0.0f,   -10.0f,     0.0f),
      Vector3(  0.0f,     0.0f,     0.0f),
      Vector3(  0.0f,   -14.0f,     0.0f),
      Vector3(  10.0f,    0.0f,     0.0f),
      Vector3(  0.0f,   -14.0f,     3.0f)
  };
  gEngine().setEnvProbeTargets(positions.data(), positions.size());
  gEngine().update();
  Texture2D* g = gRenderer().generateBRDFLUT();
  g->Save("brdf.png");
  gRenderer().freeTexture2D(g);
  gEngine().signalStop();
  gEngine().update();
#endif
  // Once done using the scene, clean it up.
  scene.cleanUp();
  // Finish.
  AssetManager::cleanUpAssets();
  // Clean up engine
  gEngine().cleanUp();
#if (_DEBUG)
  Log() << RTEXT("Game is cleaned up. Press Enter to continue...\n");
  std::cin.ignore();
#endif
  return 0;
}