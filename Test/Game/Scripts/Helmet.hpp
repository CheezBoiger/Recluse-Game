// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"

#include "Item.hpp"
#include "Game/Scene/ModelLoader.hpp"
#include "Physics/BoxCollider.hpp"
#include "../DemoTextureLoad.hpp"

// Scripts.
#include <array>
#include <algorithm>
#include <random>

using namespace Recluse;


// Helmet object example, on how to set up and update a game object for the engine.
class HelmetObject : public Item
{
  R_GAME_OBJECT(HelmetObject)
public:

    HelmetObject()
  {
  }


  void OnStart() override
  {
    SetName("Mister helmet");
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();

    m_pCollider = gPhysics().CreateBoxCollider(Vector3(0.4f, 0.5f, 0.4f));
    // m_pPhysicsComponent->SetRelativeOffset(Vector3(0.0f, 0.0f, 0.0f));
    m_pPhysicsComponent->Initialize(this);
    m_pPhysicsComponent->AddCollider(m_pCollider);
     m_pPhysicsComponent->Enable(false);

    ModelLoader::Model* model;
    ModelCache::Get("DamagedHelmet", &model);
    if (!model) Log() << "No model was found with the name: " << "DamagedHelmet!" << "\n";

    Mesh* mesh = model->meshes[0];
    //MeshCache::Get("BoomBox", &mesh);

    //MeshCache::Get("mesh_helmet_LP_13930damagedHelmet", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = model->materials[0];
#if 0
    MaterialCache::Get(
#if 0
      "BoomBox_Mat"
#else
      "Material_MR"
#endif
      , &material);
#endif
    m_pMaterialComponent->Initialize(this);
    m_pMaterialComponent->SetMaterialRef(material);

    material->SetEmissiveFactor(0.01f);
    //material->SetRoughnessFactor(1.0f);
    //material->SetMetallicFactor(1.0f);
    //material->SetEmissiveFactor(1.0f);

    m_pRendererComponent->SetMaterialComponent(m_pMaterialComponent);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    m_pRendererComponent->Initialize(this);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(0.0f, 1.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(0.5f, 0.5f, 0.5f);
    trans->Position = Vector3(dist(twist), dist(twist), dist(twist));
    //trans->Rotation = Quaternion::AngleAxis(Radians(180.0f), Vector3(1.0f, 0.0f, 0.0f));
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();
    m_factor = 0.01f;
  }

  // Updating game logic...
  void Update(r32 tick) override
  {
#define FOLLOW_CAMERA_FORWARD 0
    Transform* transform = GetTransform();
    // transform->Position += m_vRandDir * tick;
    //Quaternion q = Quaternion::AngleAxis(Radians(45.0f) * tick, Vector3(1.0f, 0.0, 0.0f));
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

    // Make emission glow.
    m_factor = Absf(sinf(static_cast<r32>(Time::CurrentTime())));
    m_pMaterialComponent->GetMaterial()->SetEmissiveFactor(m_factor);
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
  }

private:
  Vector3             m_vRandDir;
  r32                 m_factor;
};