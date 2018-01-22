#include "Game/Engine.hpp"

#include "Game/MeshComponent.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"

#include "../DemoTextureLoad.hpp"

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


// Script to move our cobe object. This is added to gameObj.
class OrbitObjectScript : public IScript {
public:
  r32     acc;

  void Awake() override
  {
    Log() << "Waking: " + GetOwner()->GetName() + "\n";
    acc = 0.0f;
    if (GetOwner()->GetParent()) {
      GetOwner()->GetTransform()->LocalScale = Vector3(0.4f, 0.4f, 0.4f);
      GetOwner()->GetTransform()->LocalPosition = Vector3(1.5f, 0.0f, 0.0f);
    }
  }

  void Update() override 
  {
#if 1
    // Test a swirling sphere...
    Transform* transform = GetOwner()->GetTransform();
    r32 sDt = static_cast<r32>(Time::DeltaTime * Time::ScaleTime);
    // If object has parent, swirl in it's local position.
    if (GetOwner()->GetParent()) {
      transform->LocalPosition.x = transform->LocalPosition.x
        - sinf(Radians(acc)) * 1.0f * sDt;
      transform->LocalPosition.y = transform->LocalPosition.y
        - cosf(Radians(acc)) * 1.0f * sDt;
      // Calculates the curvature.
      acc += 30.0f * sDt;

      transform->LocalRotation *= Quaternion::AngleAxis(Radians(20.0f * sDt), Vector3::UP);
    }
#endif
  }
};


class MoveScript : public IScript {
public:

  void Awake() override {
    Transform* transform = GetOwner()->GetTransform();
    transform->Rotation = Quaternion::AngleAxis(Radians(0.0f), Vector3::UP);
  }
  
  void Update() override {
    Transform* transform = GetOwner()->GetTransform();
    r32 sDt = static_cast<r32>(Time::DeltaTime * Time::ScaleTime);
    if (Keyboard::KeyPressed(KEY_CODE_O)) { 
      transform->Position += transform->Forward() * 5.0f * static_cast<r32>(Time::DeltaTime); 
    }
    if (Keyboard::KeyPressed(KEY_CODE_L)) { 
      transform->Position -= transform->Forward() * 5.0f * static_cast<r32>(Time::DeltaTime);
    }

    transform->Rotation *= Quaternion::AngleAxis(Radians(20.0f * sDt), Vector3::FRONT); 
  }
};


int main(int c, char* argv[])
{
  Log::DisplayToConsole(true);
  Mouse::Enable(false);
  Mouse::Show(false);
  // Start up the engine and set the input controller.
  gEngine().StartUp(RTEXT("Recluse Test Game"), false, 1200, 800);
  gEngine().SetControlInput(Controller);
  Window* window = gEngine().GetWindow();
  // Need to show the window in order to see something.
  window->Show();

  // Setting the renderer to vsync double buffering.
  {
    GpuConfigParams params;
    params._Buffering = DOUBLE_BUFFER;
    params._EnableVsync = true;
    gRenderer().UpdateRendererConfigs(&params);
  }

  // Add game object in scene.
  LoadTextures();

  Scene scene;
  GameObject* gameObj = GameObject::Instantiate();
  GameObject* obj2 = GameObject::Instantiate();
  scene.GetRoot()->AddChild(obj2);
  obj2->AddChild(gameObj);

  // Set primary light.
  {
    DirectionalLight* pPrimary = scene.GetPrimaryLight();
    pPrimary->_Ambient = Vector4(0.1f, 0.1f, 0.1f, 1.0f);
    pPrimary->_Color = Vector4(0.7f, 0.7f, 1.0f, 1.0f);
    pPrimary->_Direction = Vector4(1.0f, -1.0f, 1.0f);
    pPrimary->_Enable = true;
    pPrimary->_Intensity = 5.0f;
  }  

  // Camera set.
  FirstPersonCamera cam(Radians(45.0f), 
    static_cast<r32>(window->Width()), 
    static_cast<r32>(window->Height()), 0.001f, 1000.0f, Vector3(0.0f, 0.0f, -10.0f), Vector3(0.0f, 0.0f, 0.0f));
  cam.EnableBloom(true);
  cam.EnableAA(true);
  gEngine().SetCamera(&cam);

  // Create a mesh object and initialize it.

  Mesh mesh;
  Mesh cubeMesh;

  {
    auto vertices = UVSphere::MeshInstance(1.0f, 64, 64);
    auto indices = UVSphere::IndicesInstance(static_cast<u32>(vertices.size()), 64, 64);
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
  MeshComponent* meshComponent = gameObj->GetComponent<MeshComponent>();
  meshComponent->SetMeshRef(&mesh);

  gameObj->AddComponent<RendererComponent>();
  gameObj->AddComponent<Transform>();
  gameObj->AddComponent<OrbitObjectScript>();

  RendererComponent* rc = gameObj->GetComponent<RendererComponent>();
  rc->SetBaseMetal(0.0f);
  rc->SetBaseRough(1.0f);

  obj2->AddComponent<MeshComponent>();
  MeshComponent* m2 = obj2->GetComponent<MeshComponent>();
  m2->SetMeshRef(&cubeMesh);
  obj2->AddComponent<RendererComponent>();
  obj2->AddComponent<Transform>();

  RendererComponent* rc2 = obj2->GetComponent<RendererComponent>();
  {
    Texture2D* tex;
    TextureCache::Get(RTEXT("RustedAlbedo"), &tex);
    rc2->EnableAlbedo(true);
    rc2->SetAlbedo(tex);
    TextureCache::Get(RTEXT("RustedNormal"), &tex);
    rc2->EnableNormal(true);
    rc2->SetNormal(tex);
    TextureCache::Get(RTEXT("RustedMetal"), &tex);
    rc2->EnableMetallic(true);
    rc2->SetMetallic(tex);
    TextureCache::Get(RTEXT("RustedRough"), &tex);
    rc2->EnableRoughness(true);
    rc2->SetRoughness(tex);
    rc2->ReConfigure(); // Must call ReConfigure to update textures and mesh on renderer components.
  }
  gameObj->Wake();
  obj2->Wake();

  // Run engine, and build the scene to render.
  gEngine().Run();
  gEngine().PushScene(&scene);
  gEngine().BuildScene();

  Log() << "Timer Start: " << Time::CurrentTime() << " s\n";
  // Game loop.
  while (gEngine().Running()) {
    Time::Update();
    gEngine().Update();
    gEngine().ProcessInput();
    Log() << "FPS: " << SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime) << " fps\t\t\r";
  }
  
  // Finish.
  GameObject::DestroyAll();
  TextureCleanUp();
  mesh.CleanUp();
  cubeMesh.CleanUp();
  gEngine().CleanUp();
#if (_DEBUG)
  Log() << "Game is cleaned up. Press Enter to continue...\n";
#endif
  return 0;
}