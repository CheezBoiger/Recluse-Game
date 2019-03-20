// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/ParticleSystemComponent.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"

#include "Item.hpp"
#include "CubeObject.hpp"
#include "Game/Scene/ModelLoader.hpp"
#include "Physics/BoxCollider.hpp"
#include "Physics/SphereCollider.hpp"
#include "../DemoTextureLoad.hpp"
#include "Renderer/MeshDescriptor.hpp"

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
  void onCollision(Collision* collision) override
  {
    GameObject* other = collision->_gameObject;
    CubeObject* cube = other->castTo<CubeObject>();
    if (cube) {
      //m_pPhysicsComponent->applyImpulse(Vector3(0.0f, 1.0f, 0.0f), Vector3());  
    }
  }


  void onStartUp() override
  {
    setName("Mister helmet");
    m_pMeshComponent = new MeshComponent();
    m_pRendererComponent = new SkinnedRendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();
    m_pAnim = new AnimationComponent();
    m_pParticles = new ParticleSystemComponent();
    //m_pParticles->initialize(this);
    m_pParticles->EnableWorldSpace(true);

    m_pCollider = gPhysics().createBoxCollider(Vector3(0.4f, 0.5f, 0.4f));
    // m_pPhysicsComponent->SetRelativeOffset(Vector3(0.0f, 0.0f, 0.0f));
    m_pPhysicsComponent->initialize(this);
    m_pPhysicsComponent->setAngleFactor(Vector3(0.0f, 0.0f, 1.0f));
    m_pCollider->SetCenter(Vector3(0.0f, 0.5f, 0.0f));
    m_pPhysicsComponent->addCollider(m_pCollider);
    m_pPhysicsComponent->setEnable(false);
    ModelLoader::Model* model = nullptr;
    ModelCache::get("BrainStem", &model);
    if (!model) Log() << "No model was found with the name: " << "DamagedHelmet!" << "\n";

    Mesh* mesh = model->meshes[0];
    //MeshCache::Get("BoomBox", &mesh);

    //MeshCache::Get("mesh_helmet_LP_13930damagedHelmet", &mesh);
    m_pMeshComponent->initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = model->materials[0];
#if 0
    MaterialCache::get(
#if 0
      "BoomBox_Mat"
#else
      "RustedSample"
#endif
      , &material);
#endif
    //material->setEmissiveFactor(0.01f);

    //material->setRoughnessFactor(0.3f);
    //material->setMetallicFactor(1.0f);
    //material->setEmissiveFactor(1.0f);

    m_pRendererComponent->enableLod(false);
    m_pRendererComponent->initialize(this);
    m_pRendererComponent->forceForward(false);
    for (size_t i = 0; i < model->meshes.size(); ++i) {
      m_pRendererComponent->addMesh(model->meshes[i]);
      for (u32 j = 0; j < model->meshes[i]->getPrimitiveCount(); ++j) {
        model->meshes[i]->getPrimitive(j)->_pMat->enableEmissive(false);
      }
    }

#if 0
    // For busterDrone model work.
    for (size_t i = 0; i < model->primitives.size(); ++i) {
      ModelLoader::PrimitiveHandle& handle = model->primitives[i];
      handle.GetMaterial()->enableEmissive(true);
      handle.GetMaterial()->setEmissiveFactor(1.0f);
    }
#endif
   
    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(0.0f, 1.0f);
    Transform* trans = getTransform();
    trans->_scale = Vector3(2.0f, 2.0f, 2.0f);
    trans->_position = Vector3(dist(twist), dist(twist), dist(twist));
    //trans->_rotation = Quaternion::angleAxis(Radians(180.0f), Vector3(1.0f, 0.0f, 0.0f));
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).normalize();
    m_factor = 0.01f;

    m_pAnim->initialize(this);
    m_pRendererComponent->setAnimationHandler(m_pAnim->getAnimHandle());
    AnimClip* clip = model->animations[0];
    clip->_skeletonId = m_pMeshComponent->MeshRef()->getSkeletonReference();
    
    m_pAnim->addClip(clip, "InitialPose");
    m_pAnim->playback("InitialPose");
    m_pAnim->setPlaybackRate(0.0f);
  }

  // Updating game logic...
  void update(r32 tick) override
  {
#define FOLLOW_CAMERA_FORWARD 0
    Transform* transform = getTransform();
    // transform->_position += m_vRandDir * tick;
    //Quaternion q = Quaternion::angleAxis(Radians(45.0f) * tick, Vector3(1.0f, 0.0, 0.0f));
    //transform->_rotation = transform->_rotation * q;
#if FOLLOW_CAMERA_FORWARD
    // Have helmet rotate with camera look around.
    Quaternion targ = Camera::getMain()->getTransform()->_rotation;
    transform->_rotation = targ;
#endif
    if (Keyboard::KeyPressed(KEY_CODE_UP_ARROW)) {
      transform->_position += transform->front() * 1.0f * tick;
    }
    if (Keyboard::KeyPressed(KEY_CODE_DOWN_ARROW)) {
      transform->_position -= transform->front() * 1.0f * tick;
    }
    if (Keyboard::KeyPressed(KEY_CODE_RIGHT_ARROW)) {
      transform->_rotation = transform->_rotation * 
        Quaternion::angleAxis(Radians(45.0f) * tick, Vector3::UP);
    }
    if (Keyboard::KeyPressed(KEY_CODE_LEFT_ARROW)) {
      transform->_rotation = transform->_rotation * 
        Quaternion::angleAxis(Radians(-45.0f) * tick, Vector3::UP);
    }

    if (Keyboard::KeyPressed(KEY_CODE_V)) {
      m_pPhysicsComponent->setEnable(true);
    }

    if (Keyboard::KeyPressed(KEY_CODE_3)) {
      m_pAnim->setPlaybackRate(m_pAnim->getPlaybackRate() - tick * 0.3f);
    }

    if (Keyboard::KeyPressed(KEY_CODE_4)) {
      m_pAnim->setPlaybackRate(m_pAnim->getPlaybackRate() + tick * 0.3f);
    }

    // Make emission glow.
    m_factor = Absf(sinf(static_cast<r32>(Time::currentTime())));
  }

  void onCleanUp() override
  {
    m_pMeshComponent->cleanUp();
    m_pRendererComponent->cleanUp();
    m_pPhysicsComponent->cleanUp();
    m_pAnim->cleanUp();
    m_pParticles->cleanUp();

    delete m_pMeshComponent;
    delete m_pRendererComponent;
    delete m_pPhysicsComponent;
    delete m_pCollider;
    delete m_pAnim;
    delete m_pParticles;
  }

private:
  Vector3             m_vRandDir;
  r32                 m_factor;
  AnimationComponent*  m_pAnim;
  ParticleSystemComponent* m_pParticles;
};


#define SPHERE 1
#define DRONE 2
#define MONSTER 3
#define ENABLE_PARTICLE_TEXTURE_TEST 0
#define MODEL_TYPE DRONE
class Monster : public Item {
  R_GAME_OBJECT(Monster)
public:
  Monster() { }

  void onStartUp() override 
  {
    m_pParticleSystem = nullptr;
    m_pParticleSystem = new ParticleSystemComponent();
    m_rendererComponent.initialize(this);
    m_pParticleSystem->initialize(this);
    m_meshComponent.initialize(this);
    m_animationComponent.initialize(this);
    m_physicsComponent.initialize(this);
    m_spotLightComponent.initialize(this);

    Transform* transform = getTransform();
    m_pPhysicsComponent = &m_physicsComponent;
    m_pMeshComponent = &m_meshComponent;
    //m_pRendererComponent = &m_rendererComponent;
    m_sphereCollider = gPhysics().createSphereCollider(1.0f);
    //m_sphereCollider->SetCenter(Vector3(0.0f, 1.0f, 0.0f));
    m_physicsComponent.addCollider(m_sphereCollider);
    m_physicsComponent.setMass(0.5f);
    m_physicsComponent.setFriction(1.0f);
    m_physicsComponent.setRollingFriction(0.1f);
    m_physicsComponent.setSpinningFriction(0.1f);

    m_spotLightComponent.setOuterCutoff(cosf(Radians(25.0f)));
    m_spotLightComponent.setInnerCutoff(cosf(Radians(20.0f)));
    m_spotLightComponent.setColor(Vector4(135.0f/255.0f, 206.0f/255.0f, 250.0f/255.0f, 1.0f));
    m_spotLightComponent.setIntensity(5.0f);
    m_spotLightComponent.setOffset(Vector3(0.0f, 7.0f, 0.0f));
    m_spotLightComponent.setRotationOffset(Quaternion::angleAxis(Radians(90.0f), Vector3::RIGHT));
    m_spotLightComponent.enableFixed(true);
    m_spotLightComponent.setEnable(false);

#if !defined FORCE_AUDIO_OFF
    // Testing audio.
    gAudio().loadSound("wave.mp3", true, true, false);
    m_audioChannel = gAudio().initiateSound("wave.mp3", transform->_position, 0.1f);
#endif

#if ENABLE_PARTICLE_TEXTURE_TEST
    {
      m_particleTexture = gRenderer().createTexture2DArray();
      m_particleTexture->initialize(RFORMAT_R8G8B8A8_UNORM, 128, 128, 64);
      Image img;
      img.load("Assets/World/ParticleAtlas.png");
      m_particleTexture->update(img, 8, 8);
      img.cleanUp();
      m_pParticleSystem->SetMaxParticleCount(50);
      m_pParticleSystem->SetTextureArray(m_particleTexture);
      m_pParticleSystem->SetGlobalScale(1.0f);
      m_pParticleSystem->SetBrightnessFactor(1.5f);
      m_pParticleSystem->SetFadeOut(0.0f);
      m_pParticleSystem->SetAngleRate(0.0f);
      m_pParticleSystem->SetFadeIn(0.0f);
      m_pParticleSystem->SetMaxLife(2.55f);
      m_pParticleSystem->SetAnimationScale(25.0f, 64.0f, 0.0f);
      m_pParticleSystem->UseAtlas(true);
      m_pParticleSystem->EnableSorting(false);
      m_pParticleSystem->setEnable(true);
    }
#endif
#if MODEL_TYPE == MONSTER
    ModelLoader::Model* model = nullptr;
    ModelCache::get("Monster", &model);
    ModelLoader::AnimModel* animModel = static_cast<ModelLoader::AnimModel*>(model);

    m_meshComponent.SetMeshRef(animModel->meshes[0]);
 
    // Clips don't have a skeleton to refer to, so be sure to know which skeleton to refer the clip to.
    AnimClip* clip = animModel->animations[0];
    clip->_skeletonId = m_meshComponent.MeshRef()->getSkeletonReference();
    m_animationComponent.addClip(clip, "WalkPose");
    m_animationComponent.playback("WalkPose");  

    m_rendererComponent.addMesh(model->meshes[0]);
    m_rendererComponent.setAnimationHandler(m_animationComponent.getAnimHandle());

    Material* rusted = nullptr;
    MaterialCache::get("RustedSample", &rusted);
    transform->_scale = Vector3(0.002f, 0.002f, 0.002f);
#elif MODEL_TYPE == SPHERE
    Mesh* mesh = nullptr;
    MeshCache::get("NativeSphere", &mesh);
    m_meshComponent.SetMeshRef(mesh);
    Material* mat = nullptr;
    MaterialCache::get("RustedSample", &mat);
    m_rendererComponent.addMesh(mesh);
    mat->setBaseColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    m_rendererComponent.enableDebug(false);
    m_rendererComponent.setDebugBits(DEBUG_CONFIG_IBL_BIT);
    for (i32 lod = 0; lod < Mesh::kMaxMeshLodWidth; ++lod) {
      mesh->getPrimitive(0)->_pMat = mat;
    }
    
    m_rendererComponent.forceForward(false);
    transform->_scale = Vector3(1.0f, 1.0f, 1.0f);
#elif MODEL_TYPE == DRONE
    ModelLoader::Model* model = nullptr;
    ModelCache::get("buster", &model);

    for (size_t i = 0; i < model->meshes.size(); ++i) {
      m_rendererComponent.addMesh(model->meshes[i]);
      for (size_t p = 0; p < model->meshes[i]->getPrimitiveCount(); ++p) {
        Primitive* prim = model->meshes[i]->getPrimitive(p);
        prim->_pMat->setEmissiveFactor(0.2f);
        //prim->_pMat->disableMaps(MAT_ROUGH_BIT | MAT_METAL_BIT | MAT_ALBEDO_BIT | MAT_EMIT_BIT | MAT_AO_BIT | MAT_NORMAL_BIT);
        prim->_pMat->setBaseColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        //prim->_pMat->setRoughnessFactor(1.0f);
        //prim->_pMat->setMetallicFactor(0.04f);
      }
    }
    
    AnimClip* pClip = model->animations[0];
    m_animationComponent.addClip(pClip, "StartUp");
    m_animationComponent.playback("StartUp");
    m_rendererComponent.setAnimationHandler(m_animationComponent.getAnimHandle());
    //m_rendererComponent.enableMorphTargets(true);
    m_rendererComponent.forceForward(false);
    //m_rendererComponent.setMorphIndex0(0);
    //m_rendererComponent.setMorphIndex1(1);
    
    //m_animationComponent.addClip(model->animations[0], "Dance");
    //m_rendererComponent.setAnimationHandler(m_animationComponent.getAnimHandle());
    //m_animationComponent.playback("Dance");
    //m_animationComponent.setPlaybackRate(1.0f);
    transform->_scale = Vector3(1.5f, 1.5f, 1.5f);
 #endif

    transform->_position = Vector3(2.0f, 5.0f, 0.0f);
    m_pParticleSystem->EnableWorldSpace(true);
  }

  void update(r32 tick) override
  { 
    if (Keyboard::KeyPressed(KEY_CODE_V)) {
      m_pParticleSystem->SetMaxParticleCount(100);
    }
    
#if !defined FORCE_AUDIO_OFF
    gAudio().SetChannel3DPosition(m_audioChannel, getTransform()->_position, m_physicsComponent.getRigidBody()->_velocity);
#endif
  }

  void setPosition(const Vector3& newPos)
  {
    getTransform()->_position = newPos;
  }

  void onCleanUp() override 
  {
    m_rendererComponent.cleanUp();
    m_meshComponent.cleanUp();
    m_animationComponent.cleanUp();
    m_physicsComponent.cleanUp();
    m_pParticleSystem->cleanUp();
    m_spotLightComponent.cleanUp();

    gPhysics().freeCollider(m_sphereCollider);

#if ENABLE_PARTICLE_TEXTURE_TEST
    gRenderer().freeTexture2DArray(m_particleTexture);
    m_particleTexture = nullptr;
#endif
  }

private:
#if MODEL_TYPE == MONSTER
  SkinnedRendererComponent  m_rendererComponent;
#else
  BatchRendererComponent m_rendererComponent;
#endif
  MeshComponent             m_meshComponent;
  AnimationComponent        m_animationComponent;
  SpotLightComponent        m_spotLightComponent;
  PhysicsComponent          m_physicsComponent;
  SphereCollider*           m_sphereCollider;
  ParticleSystemComponent*  m_pParticleSystem;
  Material*                 m_pMaterialRef;
  Texture2DArray*           m_particleTexture;
#if !defined FORCE_AUDIO_OFF
  u32                       m_audioChannel;
#endif
};