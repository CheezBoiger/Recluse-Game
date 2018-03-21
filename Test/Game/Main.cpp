#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"

#include "../DemoTextureLoad.hpp"

// Scripts.
#include "Scripts/MoveObjectScript.hpp"
#include "Scripts/OrbitObjectScript.hpp"
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


class ExplodeScript : public IScript {
public:
  r32     speed;
  Vector3 direction;
  r32     bExplosionTriggered;

  void Awake() override {
    std::random_device device;
    std::mt19937 twist(device());
    std::uniform_real_distribution<r32> uni(-1.0f, 1.0f);

    GetOwner()->GetTransform()->Position = Vector3();
    GetOwner()->GetTransform()->Scale = Vector3(5.0f, 5.0f, 5.0f);
    speed = uni(twist) * 10.0f;
    direction = Vector3(uni(twist), uni(twist), uni(twist)).Normalize();
    bExplosionTriggered = false;
  }

  void Update() override {
    if (Keyboard::KeyPressed(KEY_CODE_Y)) {
      bExplosionTriggered = true;
    }

    if (bExplosionTriggered) {
      r32 s = speed * static_cast<r32>(Time::FixTime * Time::ScaleTime);
      GetOwner()->GetComponent<Transform>()->Position += direction * s;
    }
  }
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
    params._Shadows = SHADOWS_HIGH;

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

  // Create scene.
  Scene scene;
#if defined(MANUAL_INIT)
  // Add game objects into scene. This demonstrates parent-child transformation as well.
  GameObject* gameObj = GameObject::Instantiate();
  GameObject* obj2 = GameObject::Instantiate();
  scene.GetRoot()->AddChild(obj2);
  obj2->AddChild(gameObj);

  // Set primary light.
  {
    DirectionalLight* pPrimary = scene.GetPrimaryLight();
    pPrimary->_Ambient = Vector4(0.1f, 0.1f, 0.4f, 1.0f);
    pPrimary->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector3(0.01f, -1.0f, 0.0f).Normalize();
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 5.0f;
  }

  // Create a mesh object and initialize it.
  Mesh mesh;
  Mesh cubeMesh;
  Material objMat;
  Material obj2Mat;

  objMat.Initialize();
  obj2Mat.Initialize();

  {
    // Increasing segments improves the sphere's quality
    const u32 kSegments = 64;
    auto vertices = UVSphere::MeshInstance(1.0f, kSegments, kSegments);
    auto indices = UVSphere::IndicesInstance(static_cast<u32>(vertices.size()), kSegments, kSegments);
    mesh.Initialize(vertices.size(), sizeof(StaticVertex), vertices.data(), true, indices.size(), indices.data()); 
  }
  
  {
    auto vertices = Cube::MeshInstance();
    auto indices = Cube::IndicesInstance();
    cubeMesh.Initialize(vertices.size(), sizeof(StaticVertex), vertices.data(), true, indices.size(), indices.data());
  }

  // Add component stuff.
  gameObj->SetName("Cube");
  obj2->AddComponent<MoveScript>();
  gameObj->AddComponent<MeshComponent>();
  gameObj->AddComponent<MaterialComponent>();
  gameObj->GetComponent<MaterialComponent>()->SetMaterialRef(&objMat);
  MeshComponent* meshComponent = gameObj->GetComponent<MeshComponent>();
  meshComponent->SetMeshRef(&mesh);

  gameObj->AddComponent<RendererComponent>();
  gameObj->AddComponent<Transform>();
  gameObj->AddComponent<OrbitObjectScript>();

  obj2->AddComponent<MeshComponent>();
  MeshComponent* m2 = obj2->GetComponent<MeshComponent>();
  m2->SetMeshRef(&cubeMesh);
  obj2->AddComponent<MaterialComponent>();
  obj2->GetComponent<MaterialComponent>()->SetMaterialRef(&obj2Mat);
  obj2->AddComponent<RendererComponent>();
  obj2->AddComponent<Transform>();

#define objects 400
  std::array<GameObject*, objects> gameObjs;
  Material objsMat; objsMat.Initialize();
  objsMat.SetBaseMetal(0.7f);
  objsMat.SetBaseRough(0.7f);
  objsMat.SetBaseColor(Vector4(0.9f, 0.2f, 0.2f, 1.0f));
  {
    std::random_device device;
    std::mt19937 twist(device());
    std::uniform_real_distribution<r32> uni(-500.0f, 500.0f);

    for (size_t i = 0; i <  gameObjs.size(); ++i) {
      GameObject* obj = gameObjs[i];
      obj = GameObject::Instantiate();
      obj->AddComponent<Transform>();
      obj->AddComponent<RendererComponent>();
      obj->AddComponent<MeshComponent>();
      MeshComponent* meshC = obj->GetComponent<MeshComponent>();
      meshC->SetMeshRef(&mesh);
      obj->AddComponent<MaterialComponent>();
      obj->GetComponent<MaterialComponent>()->SetMaterialRef(&objsMat);
      RendererComponent* rendererC = obj->GetComponent<RendererComponent>();
      obj->AddComponent<ExplodeScript>();
      rendererC->ReConfigure();
      Transform* transform = obj->GetComponent<Transform>();
      transform->Position = Vector3(uni(twist), uni(twist), uni(twist));
      scene.GetRoot()->AddChild(obj);
      obj->Wake();
    }
  }

  // Second scene, to demonstrate the renderer's capabilities of transitioning multiple scenes.
  Scene scene2;
  scene2.GetRoot()->AddChild(obj2);

  // Set primary light.
  {
    DirectionalLight* pPrimary = scene2.GetPrimaryLight();
    pPrimary->_Ambient = Vector4(0.3f, 0.3f, 0.66f, 1.0f);
    pPrimary->_Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector3(1.0f, -1.0f, 1.0f).Normalize();
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 5.0f;
  }

  // Wake up objects
  gameObj->Wake();
  obj2->Wake();

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

    if (Keyboard::KeyPressed(KEY_CODE_J)) {
      obj2->GetComponent<MeshComponent>()->SetMeshRef(&mesh);
      obj2->GetComponent<RendererComponent>()->ReConfigure();
    }

    if (Keyboard::KeyPressed(KEY_CODE_K)) {
      obj2->GetComponent<MeshComponent>()->SetMeshRef(&cubeMesh);
      obj2->GetComponent<RendererComponent>()->ReConfigure();
    }


    gEngine().Update();
    Log() << "FPS: " << SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime) << " fps\t\t\r";
  }
  
  // Finish.
  GameObject::DestroyAll();
  TextureCleanUp();

#if defined(MANUAL_INIT)
  // Clean up of resources is required.
  mesh.CleanUp();
  objMat.CleanUp();
  obj2Mat.CleanUp();
  objsMat.CleanUp();
  cubeMesh.CleanUp();
#endif // MANUAL_INIT

  // Clean up engine
  gEngine().CleanUp();

#if (_DEBUG)
  Log() << "Game is cleaned up. Press Enter to continue...\n";
  std::cin.ignore();
#endif
  return 0;
}