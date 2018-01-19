#include "Game/Engine.hpp"

#include "Game/MeshComponent.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"

using namespace Recluse;


void Controller()
{
  Camera* cam = gEngine().GetCamera();
  if (Keyboard::KeyPressed(KEY_CODE_ESCAPE)) { gEngine().SignalStop(); }
  if (Keyboard::KeyPressed(KEY_CODE_A)) { cam->Move(Camera::LEFT, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_D)) { cam->Move(Camera::RIGHT, Time::DeltaTime); } 
  if (Keyboard::KeyPressed(KEY_CODE_W)) { cam->Move(Camera::FORWARD, Time::DeltaTime); }
  if (Keyboard::KeyPressed(KEY_CODE_S)) { cam->Move(Camera::BACK, Time::DeltaTime); }

  if (Keyboard::KeyPressed(KEY_CODE_LEFT_ARROW)) { Time::ScaleTime -= 4.0 * Time::DeltaTime; }
  if (Keyboard::KeyPressed(KEY_CODE_RIGHT_ARROW)) { Time::ScaleTime += 4.0 * Time::DeltaTime; } 
}

int main(int c, char* argv[])
{
  Log::DisplayToConsole(true);
  Mouse::Enable(false);
  // Start up the engine and set the input controller.
  gEngine().StartUp(RTEXT("Recluse Test Game"), false, 1200, 800);
  gEngine().SetControlInput(Controller);
  Window* window = gEngine().GetWindow();
  // Need to show the window in order to see something.
  window->Show();

  // Add game object in scene.
  Scene scene;
  GameObject* gameObj = GameObject::Instantiate();
  scene.GetRoot()->AddChild(gameObj);

  // Set primary light.
  {
    DirectionalLight* pPrimary = scene.GetPrimaryLight();
    pPrimary->_Ambient = Vector4(0.1f, 0.1f, 0.1f, 1.0f);
    pPrimary->_Color = Vector4(0.7f, 0.7f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector4(1.0f, -1.0f, 1.0f);
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 15.0f;
  }  

  // Camera set.
  Camera cam(Camera::PERSPECTIVE, Radians(45.0f), 
    static_cast<r32>(window->Width()), 
    static_cast<r32>(window->Height()), 0.001f, 1000.0f, Vector3(5.0f, 5.0f, -5.0f), Vector3(0.0f, 0.0f, 0.0f));
  cam.EnableBloom(true);
  cam.EnableAA(true);
  gEngine().SetCamera(&cam);

  // Create a mesh object and initialize it.

  Mesh mesh;

  {
    auto vertices = UVSphere::MeshInstance(1.0f, 64, 64);
    auto indices = UVSphere::IndicesInstance(static_cast<u32>(vertices.size()), 64, 64);
    mesh.Initialize(vertices.size(), sizeof(StaticVertex), vertices.data(), true, indices.size(), indices.data()); 
  }

  // Add component stuff.
  gameObj->AddComponent<MeshComponent>();
  MeshComponent* meshComponent = gameObj->GetComponent<MeshComponent>();
  meshComponent->SetMeshRef(&mesh);

  gameObj->AddComponent<RendererComponent>();
  gameObj->AddComponent<Transform>();

  RendererComponent* rc = gameObj->GetComponent<RendererComponent>();
  rc->SetBaseMetal(0.5f);
  rc->SetBaseRough(0.5f);

  Transform* transform = gameObj->GetComponent<Transform>();
  transform->Position = Vector3(-3.0f, 0.0f, 0.0f);

  // Run engine, and build the scene to render.
  gEngine().Run();
  gEngine().PushScene(&scene);
  gEngine().BuildScene();

  r32 acc = 0.0f;

  // Game loop.
  while (gEngine().Running()) {
    Time::Update();

    // Test a swirling sphere...
    transform->Position.x = transform->Position.x 
      + sinf(Radians(acc)) * 1.0f * static_cast<r32>(Time::DeltaTime * Time::ScaleTime);
    transform->Position.z = transform->Position.z 
      + cosf(Radians(acc)) * 1.0f * static_cast<r32>(Time::DeltaTime * Time::ScaleTime);
    // Calculates the curvature.
    acc += 20.0f * static_cast<r32>(Time::DeltaTime * Time::ScaleTime);

    gEngine().Update();
    gEngine().ProcessInput();
  }
  
  // Finish.
  GameObject::DestroyAll();
  mesh.CleanUp();
  gEngine().CleanUp();
  return 0;
}