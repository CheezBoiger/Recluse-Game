// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
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


// Helmet object example, on how to set up and update a game object for the engine.
class HelmetObject : public GameObject
{
public:

  R_GAME_OBJECT(HelmetObject)

    HelmetObject()
  {
    SetName("Mister helmet");
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();

    m_pCollider = gPhysics().CreateBoxCollider(Vector3(0.4f, 0.5f, 0.4f));
    m_pPhysicsComponent->SetRelativeOffset(Vector3(0.0f, -0.1f, 0.0f));
    m_pPhysicsComponent->SetCollider(m_pCollider);
    m_pPhysicsComponent->Initialize(this);

    Mesh* mesh = nullptr;
    //MeshCache::Get("mesh_helmet_LP_13930damagedHelmet", &mesh);
    MeshCache::Get("mesh_helmet_LP_13930damagedHelmet", &mesh);
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
    material->SetRoughnessFactor(1.0f);
    material->SetMetallicFactor(1.0f);
    material->SetEmissiveFactor(1.0f);
    m_pRendererComponent->SetMaterialComponent(m_pMaterialComponent);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    m_pRendererComponent->Initialize(this);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(0.0f, 1.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(0.5f, 0.5f, 0.5f);
    trans->Position = Vector3(dist(twist), dist(twist), dist(twist));
    trans->Rotation = Quaternion::AngleAxis(Radians(45.0f), Vector3(1.0f, 0.0f, 0.0f));
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();

    m_pPhysicsComponent->Enable(false);
  }


  void Awake() override
  {
  }

  // Updating game logic...
  void Update(r32 tick) override
  {
#define FOLLOW_CAMERA_FORWARD 0
    Transform* transform = GetTransform();
    // transform->Position += m_vRandDir * tick;
    //Quaternion q = Quaternion::AngleAxis(Radians(0.1f), Vector3(0.0f, 1.0, 0.0f));
    //transform->Rotation = transform->Rotation * q;
#if FOLLOW_CAMERA_FORWARD
    // Have helmet rotate with camera look around.
    Quaternion targ = Camera::GetMain()->GetTransform()->Rotation;
    transform->Rotation = targ;
#endif
    if (Keyboard::KeyPressed(KEY_CODE_UP_ARROW)) {
      transform->Position.x += 1.0f * tick;
    }
    if (Keyboard::KeyPressed(KEY_CODE_DOWN_ARROW)) {
      transform->Position.x -= 1.0f * tick;
    }
    if (Keyboard::KeyPressed(KEY_CODE_RIGHT_ARROW)) {
      transform->Position.z += 1.0f * tick;
    }
    if (Keyboard::KeyPressed(KEY_CODE_LEFT_ARROW)) {
      transform->Position.z -= 1.0f * tick;
    }

    if (Keyboard::KeyPressed(KEY_CODE_V)) {
      m_pPhysicsComponent->Enable(true);
    }

    if (Keyboard::KeyPressed(KEY_CODE_B) && Keyboard::KeyPressed(KEY_CODE_SHIFT)) {
      m_pPhysicsComponent->ApplyImpulse(m_vRandDir, Vector3(0.0f, 0.0f, 0.0f));
    }
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
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  MaterialComponent*  m_pMaterialComponent;
  PhysicsComponent*   m_pPhysicsComponent;
  Collider*           m_pCollider;
};