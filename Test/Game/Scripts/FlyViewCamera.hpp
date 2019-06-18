// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once
#include "Game/Engine.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Geometry/UVSphere.hpp"
#include "Renderer/UserParams.hpp"


#include "Game/Scene/ModelLoader.hpp"
#include "Physics/BoxCollider.hpp"
#include "../DemoTextureLoad.hpp"

#include "Item.hpp"
#include "Helmet.hpp"
#include "CubeObject.hpp"
#include "Lantern.hpp"

// Scripts.
#include <array>
#include <algorithm>
#include <random>

using namespace Recluse;


struct CameraTransition {
  r32 interpolate;    // smoothness.
  r32 time;           // how long to remain in this position.
  Vector3 position;   // _position of this transition.
  Quaternion rotation;  // rotated face of this transition.
};


// Main camera is an object in the scene.
class MainCamera : public GameObject
{
public:
  MainCamera()
    : m_xSensitivity(25.0f / 10000.0f)
    , m_ySensitivity(25.0f / 10000.0f)
    , m_yaw(0.0f)
    , m_pitch(0.0f)
    , bFirstLook(true)
    , m_constrainPitch(Radians(90.0f))
    , m_lastX(0.0f)
    , m_lastY(0.0f)
    , m_speed(5.0f)
    , _pHolding(nullptr)
  {
  }

  void onStartUp() override
  {
    Window* pWindow = gEngine().getWindow();
    Transform* transform = getTransform();
    // Camera set.
    pCam = new Camera(Camera::PERSPECTIVE, Radians(60.0f), 0.2f, 2000.0f);
    pCam->initialize(this);
    pCam->enableBloom(true);
    Camera::setMain(pCam);

    transform->_position = Vector3(10.0f, 10.0f, 10.0f);
    Vector3 dir = Vector3(0.0f, 0.0f, 0.0f) - transform->_position;
    transform->_rotation = Quaternion::lookRotation(dir, Vector3::UP);
    Vector3 euler = transform->_rotation.toEulerAngles();
    m_pitch = euler.x;
    m_yaw = euler.y;
    m_roll = euler.z;

    // Anything in contact with this flying camera will get pushed aside.
    m_pCollider = gPhysics().createBoxCollider(Vector3(1.0f, 1.0f, 1.0f));
    m_pPhysicsComponent = new PhysicsComponent();
    m_pPhysicsComponent->initialize(this);
    m_pPhysicsComponent->addCollider(m_pCollider);

    bFollow = false;
    m_pPhysicsComponent->setMass(0.0f);

    m_pSpotLight = new SpotLightComponent();
    m_pSpotLight->initialize(this);
    m_pSpotLight->setOuterCutoff(cosf(Radians(25.0f)));
    m_pSpotLight->setInnerCutoff(cosf(Radians(24.0f)));
    m_pSpotLight->setColor(Vector4(135.0f/255.0f, 206.0f/255.0f, 250.0f/255.0f, 1.0f));
    m_pSpotLight->setIntensity(2.0f);
    m_pSpotLight->setOffset(Vector3(0.5f, -0.5f, 0.0f));

    m_pSpotLight2 = new SpotLightComponent();
    m_pSpotLight2->initialize(this);
    m_pSpotLight2->setOuterCutoff(cosf(Radians(25.0f)));
    m_pSpotLight2->setInnerCutoff(cosf(Radians(24.0f)));
    m_pSpotLight2->setColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    m_pSpotLight2->setIntensity(2.0f);
    m_pSpotLight2->setOffset(Vector3(-0.5f, -0.5f, 0.0f));
    pCam->enableFilmGrain(true);
    pCam->setFilmGrainSpeed(50.0f);
  }

  // Game object updating.
  void update(r32 tick) override
  {
    if (Keyboard::KeyPressed(KEY_CODE_ESCAPE)) { gEngine().signalStop(); }
    Transform* transform = getTransform();

    if (Keyboard::KeyPressed(KEY_CODE_0)) {
     // pCam->setFoV(pCam->getFoV() + Radians(1.0f));
     //pCam->setExposure(pCam->getExposure() - 2.0f * Time::deltaTime);
      //gRenderer().takeSnapshot("screenshot.png");
      m_pSpotLight->setEnable(false);
      m_pSpotLight2->setEnable(false);
    }

    if (Keyboard::KeyPressed(KEY_CODE_1)) {
      //pCam->setFoV(pCam->getFoV() - Radians(1.0f));
      //pCam->setExposure(pCam->getExposure() + 2.0f * (r32)Time::deltaTime);
      m_pSpotLight->setEnable(true);
      m_pSpotLight2->setEnable(true);
    }

    if (!bFollow) {
      r32 speed = m_speed;
      if (Keyboard::KeyPressed(KEY_CODE_LSHIFT)) { speed *= 5.0f; }
      if (Keyboard::KeyPressed(KEY_CODE_A)) { transform->_position -= transform->right() * speed * tick; }
      if (Keyboard::KeyPressed(KEY_CODE_D)) { transform->_position += transform->right() * speed * tick; }
      if (Keyboard::KeyPressed(KEY_CODE_W)) { transform->_position += transform->front() * speed * tick; }
      if (Keyboard::KeyPressed(KEY_CODE_S)) { transform->_position -= transform->front() * speed * tick; }
    }

    // Test window resizing.
    if (Keyboard::KeyPressed(KEY_CODE_N)) { 
      GraphicsConfigParams params = gRenderer().getCurrentGraphicsConfigs();
      params._Resolution = Resolution_1920x1080;
      gRenderer().updateRendererConfigs(&params);
      gEngine().getWindow()->setToWindowed(Window::getFullscreenWidth(), Window::getFullscreenHeight(), true); 
    }

    if (Keyboard::KeyPressed(KEY_CODE_M)) { 
      GraphicsConfigParams params = gRenderer().getCurrentGraphicsConfigs();
      params._Resolution = Resolution_1200x800;
      gRenderer().updateRendererConfigs(&params);
      gEngine().getWindow()->setToWindowed(1200, 800); 
      gEngine().getWindow()->setToCenter();
      gEngine().getWindow()->show(); 
    }

    // Testing renderer configurations during runtime.
    if (Keyboard::KeyPressed(KEY_CODE_8)) {
      GraphicsConfigParams params = gRenderer().getCurrentGraphicsConfigs();
      params._Buffering = DOUBLE_BUFFER;
      params._EnableVsync = true;
      params._EnableBloom = false;
      params._AA = AA_None;
      params._Shadows = GRAPHICS_QUALITY_NONE;
      params._TextureQuality = GRAPHICS_QUALITY_ULTRA;
      params._EnableMultithreadedRendering = true;
      gRenderer().updateRendererConfigs(&params);
    }

    if (Keyboard::KeyPressed(KEY_CODE_9)) {
      GraphicsConfigParams params = gRenderer().getCurrentGraphicsConfigs();
      params._Buffering = DOUBLE_BUFFER;
      params._EnableVsync = true;
      params._EnableBloom = true;
      params._EnableMultithreadedRendering = false;
      params._AA = AA_FXAA_2x;
      params._Shadows = GRAPHICS_QUALITY_HIGH;
      params._TextureQuality = GRAPHICS_QUALITY_ULTRA;
      gRenderer().updateRendererConfigs(&params);
    }

    if (bFirstLook) {
      m_lastX = (r32)Mouse::getX();
      m_lastY = (r32)Mouse::getY();
      bFirstLook = false;
    }

    if (Mouse::isTracking()) {
      r32 xoffset = m_lastX - (r32)Mouse::getX();
      r32 yoffset = (r32)Mouse::getY() - m_lastY;
      m_lastX = (r32)Mouse::getX();
      m_lastY = (r32)Mouse::getY();

      xoffset *= m_xSensitivity;
      yoffset *= m_ySensitivity;

      m_yaw += xoffset;
      m_pitch += yoffset;
      if (m_pitch < -Radians(270.0f)) {
        m_pitch = -Radians(270.0f);
      }
      if (m_pitch > -Radians(90.0f)) {
        m_pitch = -Radians(90.0f);
      }
    }
    Vector3 euler = Vector3(m_pitch, m_yaw, m_roll);
    if (!bFollow) {
      transform->_rotation = Quaternion::eulerAnglesToQuaternion(euler);
    }

    // Must update the camera manually, as it may need to be updated before other game logic.
    // Update before ray picking.
    pCam->update();

#if !defined FORCE_AUDIO_OFF
    gAudio().setListener3DOrientation(
      transform->_position,
      transform->front(),
      transform->up());
#endif

    // Testing ray cast.
    if (Mouse::buttonDown(Mouse::LEFT)) {
      RayTestHit hitOut;
      if (gPhysics().rayTest(transform->_position, transform->front(), 50.0f, &hitOut)) {
        GameObject* obj = hitOut._rigidbody->_gameObj;
        Item* item = obj->castTo<Item>();
        if (item) {
          _pHolding = item;
          item->GetPhysicsComponent()->setMass(0.0f);
        }
      }
    }

    if (Keyboard::KeyPressed(KEY_CODE_E) && _pHolding) {
      // Let go of object we are holding.
      _pHolding->GetPhysicsComponent()->setMass(1.0f);
      _pHolding->GetPhysicsComponent()->reset();
      _pHolding = nullptr;
    }

    if (_pHolding) {
      Transform* t = _pHolding->getTransform();
      t->_position = transform->_position + transform->front() * 3.0f;
    }

#define CAMERA_REVOLVE 0
#if CAMERA_REVOLVE > 0
    t += tick * 0.2f;
    Vector3 xPos = Vector3(cosf(t) * 10.0f, 0.0f, 0.0f);
    Vector3 yPos = Vector3(0.0f, 0.0f, sinf(t) * 10.0f);
    transform->_position = xPos + yPos + Vector3::UP * 10.0f;
    Vector3 dir = Vector3(0.0f, 0.0f, 0.0f) - transform->_position;
    transform->_rotation = Quaternion::lookRotation(dir.normalize(), Vector3::UP);
#endif
  }

  void onCleanUp() override 
  {
    m_pPhysicsComponent->cleanUp();
    m_pSpotLight->cleanUp();
    m_pSpotLight2->cleanUp();
    delete m_pPhysicsComponent;
    delete m_pCollider;
    delete m_pSpotLight;
    delete m_pSpotLight2;
  }

private:
  Camera * pCam;
  b8      bFirstLook;
  r32     m_lastX;
  r32     m_lastY;
  r32     m_xSensitivity;
  r32     m_ySensitivity;
  r32     m_pitch;
  r32     m_yaw;
  r32     m_roll;
  r32     m_constrainPitch;
  r32     m_speed;
#if CAMERA_REVOLVE > 0
  r32     t = 0.0f;
#endif
  PhysicsComponent* m_pPhysicsComponent;
  SpotLightComponent* m_pSpotLight;
  SpotLightComponent* m_pSpotLight2;

  Collider*         m_pCollider;
  b32               bFollow;
  // Object to hold on to.
  Item*             _pHolding;
  Transform         oldTransform;
};