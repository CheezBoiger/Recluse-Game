// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"


#include "Game/Scene/ModelLoader.hpp"
#include "../DemoTextureLoad.hpp"
#include "Scripts/FlyViewCamera.hpp"

// Scripts.
#include <array>
#include <algorithm>
#include <random>

using namespace Recluse;


// Lantern handle example, on how to set up and update a game object for the engine.
class LanternCage : public GameObject
{
public:

  LanternCage()
  {
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();

    Mesh* mesh = nullptr;
    MeshCache::Get("lantern supporting_cage", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = nullptr;
    MaterialCache::Get(
#if 0
      "StingrayPBS1SG"
#else
      "Material_MR"
#endif
      , &material);
    m_pMaterialComponent->SetMaterialRef(material);
    m_pMaterialComponent->Initialize(this);
    //material->SetEmissiveFactor(1.0f);
    //material->SetRoughnessFactor(1.0f);
    m_pRendererComponent->SetMaterialComponent(m_pMaterialComponent);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    m_pRendererComponent->Initialize(this);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(0.01f, 0.01f, 0.01f);
    trans->Position = Vector3(0.0f, 0.05f, 0.0f);
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();
  }


  void Awake() override
  {
  }

  void Update(r32 tick) override
  {
  }

  void CleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pMaterialComponent->CleanUp();
    m_pRendererComponent->CleanUp();

    delete m_pMeshComponent;
    delete m_pMaterialComponent;
    delete m_pRendererComponent;
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  MaterialComponent*  m_pMaterialComponent;
};


// Lantern handle example, on how to set up and update a game object for the engine.
class LanternHandle : public GameObject
{
public:

  LanternHandle()
  {
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();

    Mesh* mesh = nullptr;
    MeshCache::Get("lantern handle", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = nullptr;
    MaterialCache::Get(
#if 0
      "StingrayPBS1SG"
#else
      "RustySample"
#endif
      , &material);
    m_pMaterialComponent->SetMaterialRef(material);
    m_pMaterialComponent->Initialize(this);
    //material->SetEmissiveFactor(1.0f);
    //material->SetRoughnessFactor(1.0f);
    m_pRendererComponent->SetMaterialComponent(m_pMaterialComponent);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    m_pRendererComponent->Initialize(this);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(0.01f, 0.01f, 0.01f);
    trans->Position = Vector3(0.0f, 0.05f, 0.0f);
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();
  }


  void Awake() override
  {
  }

  void Update(r32 tick) override
  {
  }

  void CleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pMaterialComponent->CleanUp();
    m_pRendererComponent->CleanUp();

    delete m_pMeshComponent;
    delete m_pMaterialComponent;
    delete m_pRendererComponent;
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  MaterialComponent*  m_pMaterialComponent;
};


// Lantern object example, on how to set up and update a game object for the engine.
// Lantern contains other children game objects to demonstrate parent-child transformations.

// TODO(): Do we want multiple render components in one game object?
class LanternObject : public GameObject
{
public:

  LanternObject()
  {
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();
    m_pCollider = gPhysics().CreateBoxCollider(Vector3(0.4f, 1.0f, 0.4f));

    m_pPhysicsComponent->SetCollider(m_pCollider);
    m_pPhysicsComponent->Initialize(this);
    m_pPhysicsComponent->SetRelativeOffset(Vector3(0.0f, 1.0f, 0.0f));

    Mesh* mesh = nullptr;
    MeshCache::Get("lantern lantern_base", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = nullptr;
    MaterialCache::Get(
#if 0
      "StingrayPBS1SG"
#else
      "RustySample"
#endif
      , &material);
    m_pMaterialComponent->SetMaterialRef(material);
    m_pMaterialComponent->Initialize(this);
    //material->SetEmissiveFactor(1.0f);
    //material->SetRoughnessFactor(1.0f);
    //m_pRendererComponent->SetMaterialComponent(m_pMaterialComponent);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    m_pRendererComponent->Initialize(this);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(0.01f, 0.01f, 0.01f);
    trans->Position = Vector3(-4.0f, 5.0f, 0.0f);
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();

    m_pCage = new LanternCage();
    m_pHandle = new LanternHandle();

    AddChild(m_pCage);
    AddChild(m_pHandle);
  }


  void Awake() override
  {
  }

  void Update(r32 tick) override
  {
    Transform* transform = GetTransform();
    // transform->Position += m_vRandDir * tick;
    Quaternion q = Quaternion::AngleAxis(Radians(90.0f * tick), Vector3(0.0f, 1.0, 0.0f));
    transform->Rotation = transform->Rotation * q;
  }

  void CleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pMaterialComponent->CleanUp();
    m_pRendererComponent->CleanUp();
    m_pPhysicsComponent->CleanUp();

    delete m_pMeshComponent;
    delete m_pMaterialComponent;
    delete m_pRendererComponent;
    delete m_pPhysicsComponent;
    delete m_pCollider;

    m_pCage->CleanUp();
    m_pHandle->CleanUp();

    delete m_pCage;
    delete m_pHandle;
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  MaterialComponent*  m_pMaterialComponent;
  PhysicsComponent*   m_pPhysicsComponent;
  Collider*           m_pCollider;

  LanternCage*        m_pCage;
  LanternHandle*      m_pHandle;
};