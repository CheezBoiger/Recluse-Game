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


  void onStartUp() override
  {
    m_pMeshComponent = new MeshComponent();
    m_pRendererComponent = new RendererComponent();
    m_pointLight=  new PointLightComponent();

    ModelLoader::Model* model = nullptr;
    ModelCache::get("Lantern", &model);
    Mesh* mesh = model->meshes[1];
    //MeshCache::Get("lantern handle", &mesh);
    m_pMeshComponent->initialize(this);
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
    //material->setEmissiveFactor(1.0f);
    //material->setRoughnessFactor(1.0f);
    m_pRendererComponent->initialize(this);
    m_pRendererComponent->addMesh(mesh);
    m_pRendererComponent->enableAutoLod(false);
    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = getTransform();
    trans->_scale = Vector3(0.01f, 0.01f, 0.01f);
    trans->_position = Vector3(0.0f, 0.05f, 0.0f);
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).normalize();

    m_pointLight->initialize(this);
    m_pointLight->setOffset(Vector3(-2.0f, 3.6f, 0.0f));
    m_pointLight->setColor(Vector4(1.0f, 0.5f, 0.3f, 1.0f));
    m_pointLight->setRange(10.0f);
    m_pointLight->setIntensity(2.0f);
    m_pointLight->setEnable(true);
  }

  void update(r32 tick) override
  {
    if (Keyboard::keyPressed(KEY_CODE_5)) {
      m_pointLight->enableDebug(false);
    }
    if (Keyboard::keyPressed(KEY_CODE_6)) {
      m_pointLight->enableDebug(true);;
    }
  }

  void onCleanUp() override
  {
    m_pMeshComponent->cleanUp();
    m_pRendererComponent->cleanUp();

    delete m_pMeshComponent;
    delete m_pRendererComponent;

    m_pointLight->cleanUp();
    delete m_pointLight;
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
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


  void onStartUp() override
  {
    m_pMeshComponent = new MeshComponent();
    m_pRendererComponent = new RendererComponent();

    ModelLoader::Model* model = nullptr;
    ModelCache::get("Lantern", &model);
    Mesh* mesh = model->meshes[2];
    //MeshCache::Get("lantern supporting_cage", &mesh);
    m_pMeshComponent->initialize(this);
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
    //material->setEmissiveFactor(1.0f);
    //material->setRoughnessFactor(1.0f);
    m_pRendererComponent->initialize(this);
    m_pRendererComponent->addMesh(mesh);
    m_pRendererComponent->enableAutoLod(false);
    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = getTransform();
    trans->_position = Vector3(0.0f, 0.05f, 0.0f);
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).normalize();

    m_pHandle = new LanternHandle();
    addChild(m_pHandle);
    m_pHandle->start();
  }

  void update(r32 tick) override
  {
    m_pHandle->update(tick);
  }

  void onCleanUp() override
  {
    m_pMeshComponent->cleanUp();
    m_pRendererComponent->cleanUp();

    delete m_pMeshComponent;
    delete m_pRendererComponent;

    m_pHandle->cleanUp();
    delete m_pHandle;
  }

private:
  Vector3             m_vRandDir;
  RendererComponent*  m_pRendererComponent;
  MeshComponent*      m_pMeshComponent;
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


  void onStartUp() override
  {
    setName("Lantern");
    m_pMeshComponent = new MeshComponent();
    m_pRendererComponent = new RendererComponent();
    m_pPhysicsComponent = new PhysicsComponent();
    m_pCollider = gPhysics().createBoxCollider(Vector3(0.2f, 2.5f, 0.2f));
    m_secondCollider = gPhysics().createBoxCollider(Vector3(1.5f, 0.2f, 0.2f));
    m_pCollider->SetCenter(Vector3(0.0f, 2.7f, 0.0f));
    m_secondCollider->SetCenter(Vector3(0.0f, 4.4f, 0.0f));

    m_pPhysicsComponent->initialize(this);
    m_pPhysicsComponent->addCollider(m_pCollider);
    m_pPhysicsComponent->addCollider(m_secondCollider);
    m_pPhysicsComponent->setMass(0.0f);
    // m_pPhysicsComponent->SetRelativeOffset(Vector3(0.0f, 0.45f, 0.0f));

    ModelLoader::Model* model = nullptr;
    ModelCache::get("Lantern", &model);
    Mesh* mesh = model->meshes[0];//nullptr;
    //MeshCache::Get("lantern lantern_base", &mesh);
    m_pMeshComponent->initialize(this);
    m_pMeshComponent->SetMeshRef(mesh);

    Material* material = model->materials[0];//nullptr;
/*
    MaterialCache::Get(
#if 1
      "StingrayPBS1SG"
#else
      "RustySample"
#endif
      , &material);
*/

    m_pRendererComponent->initialize(this);
    m_pRendererComponent->addMesh(mesh);
    m_pRendererComponent->enableAutoLod(false);
    material->setEmissiveFactor(0.2f);

    std::random_device r;
    std::mt19937 twist(r());
    std::uniform_real_distribution<r32> dist(-10.0f, 10.0f);
    Transform* trans = getTransform();
    trans->_scale = Vector3(0.2f, 0.2f, 0.2f);
    trans->_position = Vector3(-4.0f, 0.0f, 4.0f);
    trans->_rotation = Quaternion::angleAxis(Radians(180.0f + 0.0f), Vector3(0.0f, 1.0f, 0.0f));
    m_vRandDir = Vector3(dist(twist), dist(twist), dist(twist)).normalize();
    m_pCage = new LanternCage();
    addChild(m_pCage);
    m_pCage->start();
    bFollow = false;
  }

  void update(r32 tick) override
  {
    Camera* cam = Camera::getMain();
    Transform* camTransform = cam->getTransform();

    if (!bFollow && Keyboard::keyPressed(KEY_CODE_0)) {
      oldTransform = *camTransform;
      bFollow = false;
    }

    if (bFollow && Keyboard::keyPressed(KEY_CODE_1)) {
      *camTransform = oldTransform;
      bFollow = false;
    }

    if (bFollow) {
      Camera* cam = Camera::getMain();
      Transform* camTransform = cam->getTransform();
      Transform* transform = getTransform();
      camTransform->_position = transform->_position + transform->front() * 3.0f;
      camTransform->_rotation = transform->_rotation * Quaternion::angleAxis(Radians(180.f), Vector3::UP);
    }

    if (Keyboard::keyPressed(KEY_CODE_U)) {
      m_pRendererComponent->setEnable(false);
    }
    if (Keyboard::keyPressed(KEY_CODE_Y)) {
      m_pRendererComponent->setEnable(true);
    }

    m_pCage->update(tick);
  }

  void onCleanUp() override
  {
    m_pMeshComponent->cleanUp();
    m_pRendererComponent->cleanUp();
    m_pPhysicsComponent->cleanUp();

    delete m_pMeshComponent;
    delete m_pRendererComponent;
    delete m_pPhysicsComponent;
    delete m_pCollider;
    delete m_secondCollider;

    m_pCage->cleanUp();
    delete m_pCage;
  }

private:
  Vector3             m_vRandDir;
  b32                 bFollow;
  LanternCage*        m_pCage;
  Transform           oldTransform;
  Collider*           m_secondCollider;
};