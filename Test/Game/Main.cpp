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


// Test scene that is used for setting up the game world.
class TestScene : public Scene {
  static const U32 kMaxCount = 1;
  static const U32 kNumberOfMonsters = 1;
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

    for (U32 i = 0; i < kMaxCount; ++i) {
      helmets.push_back(new HelmetObject());
      getRoot()->addChild(helmets[i]);
      helmets[i]->start();
    }

    std::random_device dev;
    std::mt19937 twist(dev());
    std::uniform_real_distribution<R32> uni(-10.0f, 10.0f);
    std::uniform_real_distribution<R32> above(0.0f, 20.0f);
    for (U32 i = 0; i < kNumberOfMonsters; ++i) {
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
      //pSky->setSkyColor({ 0.0f, 0.0f, 0.0f });
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
      //std::vector<LightProbe> probes = LightProbeManager::loadProbesFromFile(
      //    "Assets/World/lightProbes.probe");
      //lightprobe = probes[0];

      //otherLightProbe = probes[1];
      Image img;
      img.load("Assets/World/testcubemap.png");
      cubemap0->update(img);
      img.cleanUp();
      img.load("Assets/World/Probe2.png");
      cubemap1->update(img);
      img.cleanUp();
      img.load("Assets/World/brdf.png");
      brdfLUT->update(img);
      img.cleanUp();
      lightprobe.generateSHCoefficients(gRenderer().getRHI(), cubemap0);
      lightprobe._bias = 1.0f;
      lightprobe.saveToFile("Assets/World/test.probe");
      otherLightProbe.generateSHCoefficients(gRenderer().getRHI(), cubemap1);
      otherLightProbe._bias = 1.0f;
      otherLightProbe.saveToFile("Assets/World/secondTest.probe");

      //lightprobe = probes[0];
      //otherLightProbe = probes[1];
      LightProbe probes[] = { lightprobe, otherLightProbe };
      LightProbeManager::saveProbesToFile("Assets/World/lightProbes.probe", probes, 2);

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
  void update(R32 tick) override {
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
    for (U32 i = 0; i < kMaxCount; ++i) {
      helmets[i]->cleanUp();
      delete helmets[i];
    }

    for (U32 i = 0; i < kNumberOfMonsters; ++i) {
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
  UserConfigParams userConfigs;
};


int main(int c, char* argv[])
{
  Log::displayToConsole(true);
  Mouse::setEnable(false);
  Mouse::show(false);

  // Setting the renderer to vsync double buffering when starting up the engine,
  // Inputting gpu params is optional, and can pass nullptr if you prefer default.
  // User parameters is useful for global parameters such as cameras, be sure to read and
  // set to engine global.
  {
    GraphicsConfigParams params = { };
    gEngine( ).readGraphicsConfig( params );

    UserConfigParams userParams;
    gEngine( ).readUserConfigs( userParams );
    gEngine( ).setGlobalUserConfigs( userParams );

    // Start up the engine and set the input controller.
    gEngine().startUp(RTEXT("Recluse Test Game"), &userParams, &params);
    gEngine().run();
    Window* pWindow = gEngine().getWindow();
    switch (userParams._windowType) {
    case WindowType_Borderless:
    {
      pWindow->setToWindowed(userParams._windowWidth, userParams._windowHeight, true);
      pWindow->setToCenter();
    } break;
    case WindowType_Border:
    {
      pWindow->setToWindowed(userParams._windowWidth, userParams._windowHeight);
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
    I32 stckCnt = 32;
    I32 minus = 32 / 5;
    //for (I32 lod = 0; lod < 5; ++lod) {
      auto sphereVerts = UVSphere::meshInstance(1.0f, stckCnt, stckCnt);
      auto sphereInd = UVSphere::indicesInstance(static_cast<U32>(sphereVerts.size()), stckCnt, stckCnt);
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
#if 0
  while (gEngine().isRunning()) {
    Time::update();
    gEngine().processInput();
    scene.update((R32)Time::deltaTime);
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
  g->save("brdf.png");
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