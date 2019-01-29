// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"
#include "UI/UI.hpp"

#include "Core/Utility/Profile.hpp"
#include "Item.hpp"
#include "Game/Scene/ModelLoader.hpp"
#include "Physics/BoxCollider.hpp"
#include "../DemoTextureLoad.hpp"

// Scripts.
#include <array>
#include <algorithm>
#include <random>

using namespace Recluse;


// Spehere object example, on how to set up and update a game object for the engine.
class CubeObject : public GameObject
{
  R_GAME_OBJECT(CubeObject)

  r32     m_health;
  r32     m_maxHealth;
  r32     m_healthRegen;

public:

    CubeObject()
  {
    SetName("CubeObject :3");
  }

  void OnStartUp() override
  {
    m_health = 0.f;
    m_healthRegen = 1.0f;
    m_maxHealth = 100.0f;
    m_pMeshComponent = new MeshComponent();
    m_pRendererComponent = new RendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();
    m_pCollider = gPhysics().CreateBoxCollider(Vector3(15.0f, 1.0f, 15.0f));
#if 1
    m_pCollider->SetCenter(Vector3(0.0f, -1.0f, 0.0f));
#endif
    m_pPhysicsComponent->Initialize(this);
    m_pPhysicsComponent->AddCollider(m_pCollider);
    m_pPhysicsComponent->SetMass(0.0f);
    m_pPhysicsComponent->SetFriction(1.0f);

    Mesh* mesh = nullptr;
    MeshCache::Get("NativeCube", &mesh);
    m_pMeshComponent->Initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = nullptr;
    MaterialCache::Get(
#if 0
      "GrassySample"
#else
      "RustedSample"
#endif
      , &material);
    Transform* trans = GetTransform();
    m_pRendererComponent->Initialize(this);
#if 0
    m_pRendererComponent->EnableStatic(true);
    m_pRendererComponent->AddMesh(mesh);
    m_pRendererComponent->EnableLod(false);
    mesh->GetPrimitive(0)->_pMat = material;
    trans->Scale = Vector3(15.0f, 15.0f, 15.0f);
#else
    m_pRendererComponent->ForceForward(false);
    m_pRendererComponent->EnableStatic(true);
    ModelLoader::Model* model = nullptr;
    ModelCache::Get("Sponza", &model);
    for (size_t i = 0; i < model->meshes.size(); ++i) {
      m_pRendererComponent->AddMesh(model->meshes[i]);
    }
    trans->Scale = Vector3(1.0f, 1.0f, 1.0f);
    m_pRendererComponent->EnableDebug(false);
    m_pRendererComponent->SetDebugBits(DEBUG_CONFIG_ALBEDO_BIT);
#endif
    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-4.0f, 4.0f);
    //trans->Rotation = Quaternion::AngleAxis(Radians(90.0f), Vector3(1.0f, 0.0f, 0.0f));
    trans->Position = Vector3(0.0f, -15.0f, 0.0f);
    //m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();
  }

  void Update(r32 tick) override
  {
    Transform* transform = GetTransform();
    //transform->Position += m_vRandDir * tick;
    //Quaternion q = Quaternion::AngleAxis(-Radians(0.1f), Vector3(0.0f, 0.0, 1.0f));
    //transform->Rotation = transform->Rotation * q;
    // Test sun rendering. This is not mandatory for running the engine!
#define ALLOW_SUN_MOVEMENT 1
#if ALLOW_SUN_MOVEMENT >= 1
    static r32 tt = 0.0f;
    if (Keyboard::KeyPressed(KEY_CODE_Q)) {
      Scene* scene = gEngine().GetScene();
      DirectionalLight* light = scene->GetSky()->GetSunLight();
      light->_Direction = Vector3(
        0.0f, 
        sinf(static_cast<r32>(tt * 0.01f)) * 0.5f,
        cosf(static_cast<r32>(tt * 0.01f)) * 0.5f).Normalize();
      tt += 1.0f;
    }
#endif
    AABB aabb = m_pMeshComponent->MeshRef()->GetAABB();
    aabb.min = (aabb.min * transform->Scale) + transform->Position;
    aabb.max = (aabb.max * transform->Scale) + transform->Position;
    ViewFrustum::Result result = Camera::GetMain()->GetViewFrustum().Intersect(aabb);

    std::string frustumResult = "Frustum result: ";
    ViewFrustum frustum = Camera::GetMain()->GetViewFrustum();
    Plane farPlane = frustum[ViewFrustum::PFAR];
    Plane nearPlane = frustum[ViewFrustum::PNEAR];
    Plane topPlane = frustum[ViewFrustum::PTOP];
    Plane bottomPlane = frustum[ViewFrustum::PBOTTOM];
    Plane leftPlane = frustum[ViewFrustum::PLEFT];
    Plane rightPlane = frustum[ViewFrustum::PRIGHT];
    std::string farS = "Far Plane:" + std::string(farPlane); 
    std::string nearS = "Near Plane: " + std::string(nearPlane);
    std::string topS = "Top Plane: " + std::string(topPlane);
    std::string bottomS = "Bottom Plane: " + std::string(bottomPlane);
    std::string leftS = "Left Plane: " + std::string(leftPlane);

    switch (result) {
      case ViewFrustum::Result_Inside: frustumResult += "Inside"; break;
      case ViewFrustum::Result_Intersect: frustumResult += "Intersect"; break;
      case ViewFrustum::Result_Outside: frustumResult += "Outside"; break;
      default: frustumResult += "None";
    }

    m_health += m_healthRegen * tick;
    if (m_health > m_maxHealth) m_health = m_maxHealth;
 
    Vector3 spos = Camera::GetMain()->GetWorldToScreenProjection(transform->Position);

    std::string lol = GetName() + " " + std::to_string(static_cast<u32>(m_health)) + " hp";
    // UI Testing.
    // TODO(): Need to figure out how to to create canvases instead of using one default.
    std::string str = std::to_string(SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime)) + " fps       ";
    std::string engine = RTEXT("Recluse Engine v0.0.3");
    std::string device = gRenderer().GetDeviceName();
    std::string globalTime = "time: " + std::to_string(Time::CurrentTime()) + " s";
    Window* window = gEngine().GetWindow();
    std::string intro = "WASD to move;Mouse to look; ESC to escape.";
    gUI().BeginCanvas(RTEXT("Copyright (c) 2018 Recluse Project. All rights reserved."), 0.0f, window->Height() - 300.0f, 800.0f, 500.0f);
      gUI().EmitText(str, 6.0f, 100.0f, 150.0f, 20.0f);
      gUI().EmitText(engine, 6.0f, 40.0f, 350.0f, 20.0f);
      gUI().EmitText(device, 6.0f, 60.0f, 300.0f, 20.0f);
      gUI().EmitText(intro, 6.0f, 80.0f, 450.0f, 20.0f);
      gUI().EmitText(globalTime, 6.0f, 120.0f, 500.0f, 20.0f); 
    gUI().EndCanvas();

    gUI().BeginCanvas(RTEXT("Frustum Result"), 0.0f, 100.0f, 10000.0f, 1000.0f);
      gUI().EmitText(frustumResult, 6.0f, 40.0f, 500.0f, 200.0f);
      gUI().EmitText(farS, 6.0f, 60.0f, 800.0f, 20.0f);
      gUI().EmitText(nearS, 6.0f, 80.0f, 800.0f, 20.0f);
      gUI().EmitText(topS, 6.0f, 100.0f, 800.0f, 20.0f);
      gUI().EmitText(bottomS, 6.0f, 120.0f, 1000.0f, 20.0f);
      gUI().EmitText(lol, spos.x, spos.y, 300.0f, 30.0f);
      auto data = Profiler::GetAll(PROFILE_TYPES_RENDERER);
      float offset = 120.0f;
      for (auto it : data) {
        offset += 20.0f;
        std::string tss = it._tag + ": " + std::to_string(it._total * 1000.0f) + " ms";
        gUI().EmitText(tss, 6.0f, offset, 800.0f, 20.0f); 
      }
    gUI().EndCanvas();
  }

  void OnCleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pRendererComponent->CleanUp();
    m_pPhysicsComponent->CleanUp();

    delete m_pMeshComponent;
    delete m_pRendererComponent;
    delete m_pPhysicsComponent;
    delete m_pCollider;
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  Material*           m_pMaterial;
  PhysicsComponent*   m_pPhysicsComponent;
  Collider*           m_pCollider;
};