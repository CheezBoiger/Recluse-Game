// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"


#include "Item.hpp"
#include "Game/Scene/ModelLoader.hpp"
#include "../DemoTextureLoad.hpp"
#include "Scripts/FlyViewCamera.hpp"

// Scripts.
#include <array>
#include <algorithm>
#include <random>

using namespace Recluse;

// Lantern handle example, on how to set up and update a game object for the engine.
class LanternHandle : public GameObject
{
  R_GAME_OBJECT(LanternHandle)
public:

  LanternHandle()
  {
  }


  void OnStart() override
  {
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();
    m_pointLight=  new PointLightComponent();

    ModelLoader::Model* model = nullptr;
    ModelCache::Get("Lantern", &model);
    Mesh* mesh = model->meshes[1];
    //MeshCache::Get("lantern handle", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = model->materials[0];
/*
    MaterialCache::Get(
#if 0
      "StingrayPBS1SG"
#else
      "RustySample"
#endif
      , &material);
*/
    m_pMaterialComponent->Initialize(this);
    m_pMaterialComponent->SetMaterialRef(material);
    //material->SetEmissiveFactor(1.0f);
    //material->SetRoughnessFactor(1.0f);
    m_pRendererComponent->Initialize(this);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->Native()->IndexData()->IndexCount();
    prim._pMat = material->Native();
    m_pRendererComponent->SetPrimitive(prim);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(0.01f, 0.01f, 0.01f);
    trans->Position = Vector3(0.0f, 0.05f, 0.0f);
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();

    m_pointLight->Initialize(this);
    m_pointLight->SetOffset(Vector3(-2.0f, 3.6f, 0.0f));
    m_pointLight->SetColor(Vector4(1.0f, 0.5f, 0.3f, 1.0f));
    m_pointLight->SetRange(10.0f);
    m_pointLight->SetIntensity(2.0f);
    m_pointLight->Enable(true);
  }

  void Update(r32 tick) override
  {
    if (Keyboard::KeyPressed(KEY_CODE_5)) {
      m_pointLight->Debug(false);
    }
    if (Keyboard::KeyPressed(KEY_CODE_6)) {
      m_pointLight->Debug(true);;
    }
  }

  void OnCleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pMaterialComponent->CleanUp();
    m_pRendererComponent->CleanUp();

    delete m_pMeshComponent;
    delete m_pMaterialComponent;
    delete m_pRendererComponent;

    m_pointLight->CleanUp();
    delete m_pointLight;
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  MaterialComponent*  m_pMaterialComponent;
  PointLightComponent* m_pointLight;
};


// Lantern handle example, on how to set up and update a game object for the engine.
class LanternCage : public GameObject
{
  R_GAME_OBJECT(LanternCage)
public:

  LanternCage()
  {
  }


  void OnStart() override
  {
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();

    ModelLoader::Model* model = nullptr;
    ModelCache::Get("Lantern", &model);
    Mesh* mesh = model->meshes[2];
    //MeshCache::Get("lantern supporting_cage", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = model->materials[0];
/*
    MaterialCache::Get(
#if 0
      "StingrayPBS1SG"
#else
      "RustySample"
#endif
      , &material);
*/    
    m_pMaterialComponent->SetMaterialRef(material);
    m_pMaterialComponent->Initialize(this);
    //material->SetEmissiveFactor(1.0f);
    //material->SetRoughnessFactor(1.0f);
    m_pRendererComponent->Initialize(this);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->Native()->IndexData()->IndexCount();
    prim._pMat = material->Native();
    m_pRendererComponent->SetPrimitive(prim);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = GetTransform();
    trans->Position = Vector3(0.0f, 0.05f, 0.0f);
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();

    m_pHandle = new LanternHandle();
    AddChild(m_pHandle);
  }

  void Update(r32 tick) override
  {
  }

  void OnCleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pMaterialComponent->CleanUp();
    m_pRendererComponent->CleanUp();

    delete m_pMeshComponent;
    delete m_pMaterialComponent;
    delete m_pRendererComponent;

    m_pHandle->CleanUp();
    delete m_pHandle;
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  MaterialComponent*  m_pMaterialComponent;
  LanternHandle*      m_pHandle;
};


// Lantern object example, on how to set up and update a game object for the engine.
// Lantern contains other children game objects to demonstrate parent-child transformations.

// TODO(): Do we want multiple render components in one game object?
class LanternObject : public Item
{
  R_GAME_OBJECT(LanternObject)
public:

  LanternObject()
  {
  }


  void OnStart() override
  {
    SetName("Lantern");
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();
    m_pCollider = gPhysics().CreateBoxCollider(Vector3(0.2f, 2.5f, 0.2f));
    m_secondCollider = gPhysics().CreateBoxCollider(Vector3(1.5f, 0.2f, 0.2f));
    m_pCollider->center = Vector3(0.0f, 2.7f, 0.0f);
    m_secondCollider->center = Vector3(0.0f, 4.4f, 0.0f);

    m_pPhysicsComponent->Initialize(this);
    m_pPhysicsComponent->AddCollider(m_pCollider);
    m_pPhysicsComponent->AddCollider(m_secondCollider);
    m_pPhysicsComponent->SetMass(0.0f);
    // m_pPhysicsComponent->SetRelativeOffset(Vector3(0.0f, 0.45f, 0.0f));

    ModelLoader::Model* model = nullptr;
    ModelCache::Get("Lantern", &model);
    Mesh* mesh = model->meshes[0];//nullptr;
    //MeshCache::Get("lantern lantern_base", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = model->materials[0];;//nullptr;
/*
    MaterialCache::Get(
#if 1
      "StingrayPBS1SG"
#else
      "RustySample"
#endif
      , &material);
*/
    m_pMaterialComponent->Initialize(this);
    m_pMaterialComponent->SetMaterialRef(material);

    m_pRendererComponent->Initialize(this);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->Native()->IndexData()->IndexCount();
    prim._pMat = material->Native();
    m_pRendererComponent->SetPrimitive(prim);

    material->SetEmissiveFactor(0.2f);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(0.2f, 0.2f, 0.2f);
    trans->Position = Vector3(-4.0f, 0.0f, 4.0f);
    trans->Rotation = Quaternion::AngleAxis(Radians(180.0f + 45.0f), Vector3(0.0f, 1.0f, 0.0f));
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();
    m_pCage = new LanternCage();
    AddChild(m_pCage);

    bFollow = false;
  }

  void Update(r32 tick) override
  {
    Camera* cam = Camera::GetMain();
    Transform* camTransform = cam->GetTransform();

    if (!bFollow && Keyboard::KeyPressed(KEY_CODE_0)) {
      oldTransform = *camTransform;
      bFollow = true;
    }

    if (bFollow && Keyboard::KeyPressed(KEY_CODE_1)) {
      *camTransform = oldTransform;
      bFollow = false;
    }

    if (bFollow) {
      Camera* cam = Camera::GetMain();
      Transform* camTransform = cam->GetTransform();
      Transform* transform = GetTransform();
      camTransform->Position = transform->Position + transform->Front() * 3.0f;
      camTransform->Rotation = transform->Rotation * Quaternion::AngleAxis(Radians(180.f), Vector3::UP);
    }

    if (Keyboard::KeyPressed(KEY_CODE_U)) {
      m_pRendererComponent->Enable(false);
    }
    if (Keyboard::KeyPressed(KEY_CODE_Y)) {
      m_pRendererComponent->Enable(true);
    }
  }

  void OnCleanUp() override
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
    delete m_secondCollider;

    m_pCage->CleanUp();
    delete m_pCage;
  }

private:
  Vector3             m_vRandDir;
  b32                 bFollow;
  LanternCage*        m_pCage;
  Transform           oldTransform;
  Collider*           m_secondCollider;
};