// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"
#include "UI/UI.hpp"


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

public:

    CubeObject()
  {
  }


  void OnStart() override
  {
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();
    m_pCollider = gPhysics().CreateBoxCollider(Vector3(5.0f, 5.0f, 5.0f));

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
#if 1
      "GrassySample"
#else
      "Material_MR"
#endif
      , &material);
    m_pMaterialComponent->Initialize(this);
    m_pMaterialComponent->SetMaterialRef(material);

    m_pRendererComponent->Initialize(this);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    Primitive prim;
    prim._firstIndex = 0;
    prim._indexCount = mesh->Native()->IndexData()->IndexCount();
    prim._pMat = material->Native();
    m_pRendererComponent->SetPrimitive(prim);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-4.0f, 4.0f);
    Transform* trans = GetTransform();
    trans->Rotation = Quaternion::AngleAxis(Radians(90.0f), Vector3(1.0f, 0.0f, 0.0f));
    trans->Scale = Vector3(5.0f, 5.0f, 5.0f);
    trans->Position = Vector3(0.0f, -5.0f, 0.0f);
    //m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();
  }

  void Update(r32 tick) override
  {
    Transform* transform = GetTransform();
    //transform->Position += m_vRandDir * tick;
    //Quaternion q = Quaternion::AngleAxis(-Radians(0.1f), Vector3(0.0f, 0.0, 1.0f));
    //transform->Rotation = transform->Rotation * q;
    // Test sun rendering. This is not mandatory for running the engine!
    //Scene* scene = gEngine().GetScene();
    //DirectionalLight* light = scene->GetSky()->GetSunLight();
    //light->_Direction = Vector3(
    //  sinf(static_cast<r32>(Time::CurrentTime() * 0.1)), 
    //  cosf(static_cast<r32>(Time::CurrentTime() * 0.1))).Normalize();
    AABB aabb = m_pMeshComponent->MeshRef()->Native()->GetAABB();
    aabb.min = aabb.min * transform->GetLocalToWorldMatrix();
    aabb.max = aabb.max * transform->GetLocalToWorldMatrix();
    //Log() << Camera::GetMain()->GetViewFrustum().Intersect(aabb) << "\r";

    // UI Testing.
    // TODO(): Need to figure out how to to create canvases instead of using one default.
    std::string str = std::to_string(SECONDS_PER_FRAME_TO_FPS(Time::DeltaTime)) + " fps       ";
    std::string engine = RTEXT("Recluse Engine v0.0.23");
    std::string device = gRenderer().GetDeviceName();
    Window* window = gEngine().GetWindow();
    std::string intro = "WASD to move;Mouse to look; ESC to escape.";
    gUI().BeginCanvas(RTEXT("Copyright (c) 2018 Recluse Project. All rights reserved."), 0.0f, window->Height() - 300.0f, 500.0f, 500.0f);
      gUI().EmitText(str, 30.0f, 75.0f, 150.0f, 20.0f);
      gUI().EmitText(engine, 30.0f, 30.0f, 350.0f, 20.0f);
      gUI().EmitText(device, 30.0f, 45.0f, 300.0f, 20.0f);
      gUI().EmitText(intro, 30.0f, 60.0f, 450.0f, 20.0f);
    gUI().EndCanvas();
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
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
  MaterialComponent*  m_pMaterialComponent;
  PhysicsComponent*   m_pPhysicsComponent;
  Collider*           m_pCollider;
};