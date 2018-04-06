#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"


#include "Game/Scene/ModelLoader.hpp"
#include "../DemoTextureLoad.hpp"

// Scripts.
#include <array>
#include <algorithm>
#include <random>

using namespace Recluse;

#define MANUAL_INIT 1


void Controller()
{
  Camera* cam = Camera::GetMain();
  if (Keyboard::KeyPressed(KEY_CODE_ESCAPE)) { gEngine().SignalStop(); }
  if (Keyboard::KeyPressed(KEY_CODE_A)) { cam->Move(Camera::LEFT, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_D)) { cam->Move(Camera::RIGHT, Time::DeltaTime); } 
  if (Keyboard::KeyPressed(KEY_CODE_W)) { cam->Move(Camera::FORWARD, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_S)) { cam->Move(Camera::BACK, Time::DeltaTime); }

  if (Keyboard::KeyPressed(KEY_CODE_N)) { Time::ScaleTime -= 4.0 * Time::DeltaTime; }
  if (Keyboard::KeyPressed(KEY_CODE_M)) { Time::ScaleTime += 4.0 * Time::DeltaTime; } 

  if (Keyboard::KeyPressed(KEY_CODE_T)) {
    GraphicsConfigParams config = gRenderer().CurrentGraphicsConfigs();
    config._AA = AA_None;
    gRenderer().UpdateRendererConfigs(&config);
  }

  if (Keyboard::KeyPressed(KEY_CODE_R)) {
    GraphicsConfigParams config = gRenderer().CurrentGraphicsConfigs();
    config._AA = AA_FXAA_2x;
    gRenderer().UpdateRendererConfigs(&config);
  }

}

// Spehere object example, on how to set up and update a game object for the engine.
class SphereObject : public GameObject
{
public:

  SphereObject()
  {
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();

    Mesh* mesh = nullptr;
    MeshCache::Get("mesh_helmet_LP_13930damagedHelmet", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = nullptr;
    MaterialCache::Get(
#if 0
      "RustySample"
#else
      "Material_MR"
#endif
      , &material);
    m_pMaterialComponent->SetMaterialRef(material);
    m_pMaterialComponent->Initialize(this);
    material->SetBaseEmissive(0.3f);

    m_pRendererComponent->SetMaterialComponent(m_pMaterialComponent);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    m_pRendererComponent->Initialize(this);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = GetTransform();
    trans->Rotation = Quaternion::AngleAxis(Radians(90.0f), Vector3(1.0f, 0.0f, 0.0f));
    trans->Scale = Vector3(10.0f, 10.0f, 10.0f);
    //trans->Position = Vector3(dist(twist), dist(twist), dist(twist));
    //m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();
  }


  void Awake() override 
  {
  }

  void Update(r32 tick) override
  {
    Transform* transform = GetTransform();
    //transform->Position += m_vRandDir * tick;
    Quaternion q = Quaternion::AngleAxis(Radians(0.1f), Vector3(0.0f, 0.0, 1.0f));
    transform->Rotation = transform->Rotation * q;
  }

  void CleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pMaterialComponent->CleanUp();
    m_pRendererComponent->CleanUp();
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  MaterialComponent*  m_pMaterialComponent;
};


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
    gEngine().SetControlInput(Controller);
  }

  Window* window = gEngine().GetWindow();
  // Need to show the window in order to see something.
  window->Show();
  window->SetToWindowed(Window::FullscreenWidth(), Window::FullscreenHeight(), true);

  // Add game object in scene.
  LoadTextures();

  // Camera set.
  FlyViewCamera cam(Radians(60.0f), 
    static_cast<r32>(window->Width()), 
    static_cast<r32>(window->Height()), 0.001f, 2000.0f, Vector3(0.0f, 1.0f, -10.0f), Vector3(0.0f, 0.0f, 0.0f));
  cam.SetSpeed(30.0f);
  cam.EnableBloom(true);
  gEngine().SetCamera(&cam);

  ///////////////////////////////////////////////////////////////////////////////////
  // Everything within initialization will normally be handled by Managers, for now
  // we will be demonstrating manual initialization of various objects to render and
  // control something on the display.
  ///////////////////////////////////////////////////////////////////////////////////

  {
    Mesh* mesh = new Mesh();
    u32 g = 32;
    auto boxVerts = UVSphere::MeshInstance(1.0f, g, g);
    auto boxIndic = UVSphere::IndicesInstance(static_cast<u32>(boxVerts.size()), g, g);
    mesh->Initialize(boxVerts.size(), sizeof(StaticVertex), boxVerts.data(), true, boxIndic.size(), boxIndic.data());
    MeshCache::Cache("Sphere", mesh);
  }

  ModelLoader::Model model;
  ModelLoader::Load("Assets/DamagedHelmet/DamagedHelmet.gltf", &model);

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

  // Create scene.
  Scene scene;
  
  std::vector<SphereObject*> spheres;
  #define SPHERE_COUNT 1
  for (u32 i = 0; i < SPHERE_COUNT; ++i) {
    spheres.push_back(new SphereObject());
    scene.GetRoot()->AddChild(spheres[i]);
  }
 
#if defined(MANUAL_INIT)
  // Add game objects into scene. This demonstrates parent-child transformation as well.

  // Set primary light.
  {
    DirectionalLight* pPrimary = scene.GetPrimaryLight();
    pPrimary->_Ambient = Vector4(0.1f, 0.1f, 0.4f, 1.0f);
    pPrimary->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector3(0.01f, -1.0f, 0.0f).Normalize();
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 5.0f;
  }

  // Second scene, to demonstrate the renderer's capabilities of transitioning multiple scenes.
  Scene scene2;

  // Set primary light.
  {
    DirectionalLight* pPrimary = scene2.GetPrimaryLight();
    pPrimary->_Ambient = Vector4(0.3f, 0.3f, 0.66f, 1.0f);
    pPrimary->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector3(1.0f, -1.0f, 1.0f).Normalize();
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 2.0f;
  }

#endif // defined(MANUAL_INIT)

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
    DirectionalLight* light = scene.GetPrimaryLight();

    // Test sun rendering.
    //light->_Direction = Vector3(
    //  sinf(static_cast<r32>(Time::CurrentTime() * 0.1)), 
    //  cosf(static_cast<r32>(Time::CurrentTime() * 0.1))).Normalize();
    if (Keyboard::KeyPressed(KEY_CODE_G)) {
      gEngine().PushScene(&scene2);
      gEngine().BuildScene();
    }

    if (Keyboard::KeyPressed(KEY_CODE_H)) {
      gEngine().PushScene(&scene);
      gEngine().BuildScene();
    }


    gEngine().Update();
    Log() << "FPS: " << SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime) << " fps\t\t\r";
  }
  
  for (u32 i = 0; i < SPHERE_COUNT; ++i) {
    spheres[i]->CleanUp();
    delete spheres[i];
  }
  // Finish.
  MaterialCache::CleanUpAll();
  MeshCache::CleanUpAll();
  TextureCleanUp();
  // Clean up engine
  gEngine().CleanUp();
#if (_DEBUG)
  Log() << "Game is cleaned up. Press Enter to continue...\n";
  std::cin.ignore();
#endif
  return 0;
}