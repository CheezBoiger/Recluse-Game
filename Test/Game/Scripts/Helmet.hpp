// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"

#include "Item.hpp"
#include "CubeObject.hpp"
#include "Game/Scene/ModelLoader.hpp"
#include "Physics/BoxCollider.hpp"
#include "Physics/SphereCollider.hpp"
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

  // Testing collision callbacking.
  void OnCollision(Collision* collision) override
  {
    GameObject* other = collision->_gameObject;
    CubeObject* cube = other->CastTo<CubeObject>();
    if (cube) {
      m_pPhysicsComponent->ApplyImpulse(Vector3(0.0f, 1.0f, 0.0f), Vector3());  
    }
  }


  void OnStartUp() override
  {
    SetName("Mister helmet");
    m_pMeshComponent = new MeshComponent();
    m_pRendererComponent = new SkinnedRendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();
    m_pAnim = new AnimationComponent();

    m_pCollider = gPhysics().CreateBoxCollider(Vector3(0.4f, 0.5f, 0.4f));
    // m_pPhysicsComponent->SetRelativeOffset(Vector3(0.0f, 0.0f, 0.0f));
    m_pPhysicsComponent->Initialize(this);
    m_pPhysicsComponent->AddCollider(m_pCollider);
     m_pPhysicsComponent->Enable(false);

    ModelLoader::Model* model;
    ModelCache::Get("BrainStem", &model);
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
      "RustedSample"
#endif
      , &material);
#endif
    //material->SetEmissiveFactor(0.01f);

    //material->SetRoughnessFactor(0.3f);
    //material->SetMetallicFactor(1.0f);
    //material->SetEmissiveFactor(1.0f);

    m_pRendererComponent->Initialize(this);
    for (size_t i = 0; i < model->meshes.size(); ++i) {
      m_pRendererComponent->AddMesh(model->meshes[i]);
    }

#if 0
    // For busterDrone model work.
    for (size_t i = 0; i < model->primitives.size(); ++i) {
      ModelLoader::PrimitiveHandle& handle = model->primitives[i];
      handle.GetMaterial()->EnableEmissive(true);
      handle.GetMaterial()->SetEmissiveFactor(1.0f);
    }
#endif
   
    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(0.0f, 1.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(1.0f, 1.0f, 1.0f);
    trans->Position = Vector3(dist(twist), dist(twist), dist(twist));
    //trans->Rotation = Quaternion::AngleAxis(Radians(180.0f), Vector3(1.0f, 0.0f, 0.0f));
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).Normalize();
    m_factor = 0.01f;

    m_pAnim->Initialize(this);
    m_pAnim->SetSampler(gAnimation().CreateAnimSampler());
    m_pRendererComponent->SetAnimationComponent(m_pAnim);
    AnimClip* clip = static_cast<ModelLoader::AnimModel*>(model)->animations[0];
    clip->_skeletonId = m_pMeshComponent->MeshRef()->GetSkeletonReference();
    
    m_pAnim->AddClip(clip, "InitialPose");
    m_pAnim->Playback("InitialPose");
    m_pAnim->SetPlaybackRate(0.6f);
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
      transform->Position += transform->Front() * 1.0f * tick;
    }
    if (Keyboard::KeyPressed(KEY_CODE_DOWN_ARROW)) {
      transform->Position -= transform->Front() * 1.0f * tick;
    }
    if (Keyboard::KeyPressed(KEY_CODE_RIGHT_ARROW)) {
      transform->Rotation = transform->Rotation * 
        Quaternion::AngleAxis(Radians(45.0f) * tick, Vector3::UP);
    }
    if (Keyboard::KeyPressed(KEY_CODE_LEFT_ARROW)) {
      transform->Rotation = transform->Rotation * 
        Quaternion::AngleAxis(Radians(-45.0f) * tick, Vector3::UP);
    }

    if (Keyboard::KeyPressed(KEY_CODE_V)) {
      m_pPhysicsComponent->Enable(true);
    }

    if (Keyboard::KeyPressed(KEY_CODE_3)) {
      ;
    }

    // Make emission glow.
    m_factor = Absf(sinf(static_cast<r32>(Time::CurrentTime())));

    if (Keyboard::KeyPressed(KEY_CODE_K)) {
      // NOTE(): If using SkinnedRendererComponent, this will not work for static meshes like
      // this! End up with incorrect reading to the gpu!
      ModelLoader::Model* model = nullptr;  
      ModelCache::Get("BoomBox", &model);
      m_pMeshComponent->SetMeshRef(model->meshes[0]);
      transform->Scale = Vector3(50.0f, 50.0f, 50.0f);
      m_pRendererComponent->SetTransparent(true);
    }

    if (Keyboard::KeyPressed(KEY_CODE_J)) {
      // NOTE(): If using SkinnedRendererComponent, this will not work for static meshes like
      // this! End up with incorrect reading to the gpu!
      ModelLoader::Model* model = nullptr;
      ModelCache::Get("DamagedHelmet", &model);
      m_pMeshComponent->SetMeshRef(model->meshes[0]);
      transform->Scale = Vector3(0.5f, 0.5f, 0.5f);
      m_pRendererComponent->SetTransparent(false);
    }
  }

  void OnCleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pRendererComponent->CleanUp();
    m_pPhysicsComponent->CleanUp();
    m_pAnim->CleanUp();

    delete m_pMeshComponent;
    delete m_pRendererComponent;
    delete m_pPhysicsComponent;
    delete m_pCollider;
    delete m_pAnim;
  }

private:
  Vector3             m_vRandDir;
  r32                 m_factor;
  AnimationComponent*  m_pAnim;
};


#define SPHERE 1
#define DRONE 2
#define MONSTER 3

#define MODEL_TYPE SPHERE
class Monster : public Item {
  R_GAME_OBJECT(Monster)
public:
  Monster() { }

  void OnStartUp() override 
  {
    m_rendererComponent.Initialize(this);
    m_meshComponent.Initialize(this);
    m_animationComponent.Initialize(this);
    m_physicsComponent.Initialize(this);
    Transform* transform = GetTransform();
    m_pPhysicsComponent = &m_physicsComponent;
    m_pMeshComponent = &m_meshComponent;
    m_pRendererComponent = &m_rendererComponent;
#if MODEL_TYPE == MONSTER
    ModelLoader::Model* model = nullptr;
    ModelCache::Get("Monster", &model);
    ModelLoader::AnimModel* animModel = static_cast<ModelLoader::AnimModel*>(model);

    m_meshComponent.SetMeshRef(animModel->meshes[0]);
 
    // Clips don't have a skeleton to refer to, so be sure to know which skeleton to refer the clip to.
    AnimClip* clip = animModel->animations[0];
    clip->_skeletonId = m_meshComponent.MeshRef()->GetSkeletonReference();
    m_animationComponent.AddClip(clip, "WalkPose");
    m_animationComponent.Playback("WalkPose");  
    m_animationComponent.SetSampler(gAnimation().CreateAnimSampler());

    m_rendererComponent.AddMesh(model->meshes[0]);
    m_rendererComponent.SetAnimationComponent(&m_animationComponent);

    Material* rusted = nullptr;
    MaterialCache::Get("RustedSample", &rusted);
    transform->Scale = Vector3(0.002f, 0.002f, 0.002f);
#elif MODEL_TYPE == SPHERE
    Mesh* mesh = nullptr;
    MeshCache::Get("NativeSphere", &mesh);
    m_meshComponent.SetMeshRef(mesh);
    Material* mat = nullptr;
    MaterialCache::Get("RustedSample", &mat);
    m_rendererComponent.AddMesh(mesh);
    mesh->GetPrimitive(0)->_pMat = mat->Native();
    transform->Scale = Vector3(1.0f, 1.0f, 1.0f);
#elif MODEL_TYPE == DRONE
    ModelLoader::Model* model = nullptr;
    ModelCache::Get("busterDrone", &model);
    for (size_t i = 0; i < model->meshes.size(); ++i) {
      m_rendererComponent.AddMesh(model->meshes[i]);
    }

    for (size_t i = 0; i < model->materials.size(); ++i) {
      Material* material = model->materials[i];
      material->EnableEmissive(true);
      material->SetEmissiveFactor(1.0f);
    }
 #endif



    transform->Position = Vector3(2.0f, 5.0f, 0.0f);

    m_sphereCollider = gPhysics().CreateSphereCollider(1.0f);
    m_physicsComponent.AddCollider(m_sphereCollider);
    m_physicsComponent.SetMass(0.5f);
    m_physicsComponent.SetFriction(1.0f);
    m_physicsComponent.SetRollingFriction(0.1f);
    m_physicsComponent.SetSpinningFriction(0.1f);
  }

  void Update(r32 tick) override
  { 
  }

  void SetPosition(const Vector3& newPos)
  {
    GetTransform()->Position = newPos;
  }

  void OnCleanUp() override 
  {
    m_rendererComponent.CleanUp();
    m_meshComponent.CleanUp();
    m_animationComponent.CleanUp();
    m_physicsComponent.CleanUp();

    gPhysics().FreeCollider(m_sphereCollider);
  }

private:
#if MODEL_TYPE == MONSTER
  SkinnedRendererComponent  m_rendererComponent;
#else
  RendererComponent m_rendererComponent;
#endif
  MeshComponent             m_meshComponent;
  AnimationComponent        m_animationComponent;
  PhysicsComponent          m_physicsComponent;
  SphereCollider*           m_sphereCollider;
};