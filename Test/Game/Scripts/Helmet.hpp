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


  void OnStart() override
  {
    SetName("Mister helmet");
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
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
    m_pMaterialComponent->Initialize(this);
    m_pMaterialComponent->SetMaterialRef(material);
    //material->SetEmissiveFactor(0.01f);

    //material->SetRoughnessFactor(0.3f);
    //material->SetMetallicFactor(1.0f);
    //material->SetEmissiveFactor(1.0f);

    m_pRendererComponent->Initialize(this);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
    for (size_t i = 0; i < model->primitives.size(); ++i) {
      ModelLoader::PrimitiveHandle& primHandle = model->primitives[i];
      //primHandle.SetMaterial(material);
      m_pRendererComponent->SetPrimitive(primHandle.GetPrimitive());
    }
   
    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(0.0f, 1.0f);
    Transform* trans = GetTransform();
    trans->Scale = Vector3(2.0f, 2.0f, 2.0f);
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
    m_pMaterialComponent->GetMaterial()->SetEmissiveFactor(m_factor);

    if (Keyboard::KeyPressed(KEY_CODE_K)) {
      // NOTE(): If using SkinnedRendererComponent, this will not work for static meshes like
      // this! End up with incorrect reading to the gpu!
      ModelLoader::Model* model = nullptr;  
      ModelCache::Get("BoomBox", &model);
      m_pMaterialComponent->SetMaterialRef(model->materials[0]);
      m_pMeshComponent->SetMeshRef(model->meshes[0]);
      m_pMaterialComponent->GetMaterial()->SetOpacity(0.5f);
      transform->Scale = Vector3(50.0f, 50.0f, 50.0f);
      m_pRendererComponent->SetTransparent(true);
      m_pRendererComponent->ClearPrimitives();
      Primitive prim;
      prim._firstIndex = 0;
      prim._indexCount = m_pMeshComponent->MeshRef()->Native()->IndexData()->IndexCount();
      prim._pMat = m_pMaterialComponent->GetMaterial()->Native();
      m_pRendererComponent->SetPrimitive(prim);
    }

    if (Keyboard::KeyPressed(KEY_CODE_J)) {
      // NOTE(): If using SkinnedRendererComponent, this will not work for static meshes like
      // this! End up with incorrect reading to the gpu!
      ModelLoader::Model* model = nullptr;
      ModelCache::Get("DamagedHelmet", &model);
      m_pMaterialComponent->SetMaterialRef(model->materials[0]);
      m_pMeshComponent->SetMeshRef(model->meshes[0]);
      transform->Scale = Vector3(0.5f, 0.5f, 0.5f);
      m_pRendererComponent->SetTransparent(false);
      m_pRendererComponent->ClearPrimitives();
      Primitive prim;
      prim._firstIndex = 0;
      prim._indexCount = m_pMeshComponent->MeshRef()->Native()->IndexData()->IndexCount();
      prim._pMat = m_pMaterialComponent->GetMaterial()->Native();
      m_pRendererComponent->SetPrimitive(prim);
    }
  }

  void OnCleanUp() override
  {
    m_pMeshComponent->CleanUp();
    m_pMaterialComponent->CleanUp();
    m_pRendererComponent->CleanUp();
    m_pPhysicsComponent->CleanUp();
    m_pAnim->CleanUp();

    delete m_pMeshComponent;
    delete m_pMaterialComponent;
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


class Monster : public GameObject {
  R_GAME_OBJECT(Monster)
public:
  Monster() { }

  void OnStart() override 
  {
    ModelLoader::Model* model = nullptr;
    ModelCache::Get("Monster", &model);
    ModelLoader::AnimModel* animModel = static_cast<ModelLoader::AnimModel*>(model);

    m_rendererComponent.Initialize(this);
    m_meshComponent.Initialize(this);
    m_animationComponent.Initialize(this);

    m_meshComponent.SetMeshRef(animModel->meshes[0]);

    // Clips don't have a skeleton to refer to, so be sure to know which skeleton to refer the clip to.
    AnimClip* clip = animModel->animations[0];
    clip->_skeletonId = m_meshComponent.MeshRef()->GetSkeletonReference();
    m_animationComponent.AddClip(clip, "WalkPose");
    m_animationComponent.Playback("WalkPose");  
    m_animationComponent.SetSampler(gAnimation().CreateAnimSampler());

    m_rendererComponent.SetMeshComponent(&m_meshComponent);
    m_rendererComponent.SetAnimationComponent(&m_animationComponent);

    Material* rusted = nullptr;
    MaterialCache::Get("RustedSample", &rusted);
    for (size_t i = 0; i < animModel->primitives.size(); ++i) {
      ModelLoader::PrimitiveHandle& primitiveHandle = animModel->primitives[i];
      primitiveHandle.SetMaterial(rusted);
      m_rendererComponent.SetPrimitive(primitiveHandle.GetPrimitive());
    }
    
    Transform* transform = GetTransform();
    transform->Scale = Vector3(0.002f, 0.002f, 0.002f);
    transform->Position = Vector3(2.0f, 0.5f, 0.0f);
  }

  void Update(r32 tick) override
  { 
  }

  void OnCleanUp() override 
  {
    m_rendererComponent.CleanUp();
    m_meshComponent.CleanUp();
    m_animationComponent.CleanUp();
  }
private:
  SkinnedRendererComponent  m_rendererComponent;
  MeshComponent             m_meshComponent;
  AnimationComponent        m_animationComponent;
};