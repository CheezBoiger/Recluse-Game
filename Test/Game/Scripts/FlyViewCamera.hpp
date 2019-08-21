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
  R32 interpolate;    // smoothness.
  R32 time;           // how long to remain in this position.
  Vector3 position;   // _position of this transition.
  Quaternion rotation;  // rotated face of this transition.
};


class TempExplosion : public GameObject {
public:
  TempExplosion()
    : m_outOfLife(true)
    , m_pTempLight(nullptr)
    , currLife(0.0f) { }

  void onStartUp() override
  {
    if (!m_outOfLife) {
      return;
    }
    m_pTempLight = new PointLightComponent();
    m_pTempLight->initialize(this);
    m_pTempLight->setColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    m_pTempLight->setIntensity(2.0f);
    m_pTempLight->setRange(16.0f);
    m_outOfLife = false;
    currLife = 0.5f;
  }

  
  void update(R32 tick) override 
  {
    if (m_outOfLife) {
      return;
    }
    currLife -= 1.0f * tick;
    if (currLife <= 0.0f) {
      m_pTempLight->enable(false);
      m_outOfLife = true;
    }

    if (m_outOfLife) {
      cleanUp();
    }
  }

  B32 isOutOfLife() const { return m_outOfLife; }


  void onCleanUp() override
  {
    m_pTempLight->cleanUp();
    delete m_pTempLight;
    m_pTempLight = nullptr;
  }

private:
  PointLightComponent* m_pTempLight;
  R32 currLife;
  B32 m_outOfLife;
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
    , _pHolding(nullptr)
    , m_pause(false)
  {
  }

  void onStartUp() override
  {
    const UserConfigParams& globalUserParams = gEngine( ).getGlobalUserConfigs( );
    m_xSensitivity = globalUserParams._mouseSensitivityX;
    m_ySensitivity = globalUserParams._mouseSensitivityY;
    R32 fov = globalUserParams._fieldOfView;

    Window* pWindow = gEngine().getWindow();
    Transform* transform = getTransform();
    // Camera set.
    pCam = new Camera(Camera::PERSPECTIVE, fov, 0.2f, 2000.0f);
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

    //pCam->enableFilmGrain(true);
    pCam->setFilmGrainSpeed(50.0f);
  }

  // Game object updating.
  void update(R32 tick) override
  {
    if (Keyboard::keyPressed(KEY_CODE_ESCAPE)) { gEngine().signalStop(); }
    Transform* transform = getTransform();

    if (Keyboard::keyPressed(KEY_CODE_0)) {
     // pCam->setFoV(pCam->getFoV() + Radians(1.0f));
     //pCam->setExposure(pCam->getExposure() - 2.0f * Time::deltaTime);
      //gRenderer().takeSnapshot("screenshot.png");
      m_pSpotLight->setEnable(false);
      m_pSpotLight2->setEnable(false);
    }

    if (Keyboard::keyPressed(KEY_CODE_1)) {
      //pCam->setFoV(pCam->getFoV() - Radians(1.0f));
      //pCam->setExposure(pCam->getExposure() + 2.0f * (R32)Time::deltaTime);
      m_pSpotLight->setEnable(true);
      m_pSpotLight2->setEnable(true);
    }

    // Test window resizing.
    if (Keyboard::keyPressed(KEY_CODE_N)) { 
      GraphicsConfigParams params = gRenderer().getCurrentGraphicsConfigs();
      params._Resolution = Resolution_1920x1080;
      gRenderer().updateRendererConfigs(&params);
      gEngine().getWindow()->setToWindowed(Window::getFullscreenWidth(), Window::getFullscreenHeight(), true); 
    }

    if (Keyboard::keyPressed(KEY_CODE_M)) { 
      GraphicsConfigParams params = gRenderer().getCurrentGraphicsConfigs();
      params._Resolution = Resolution_1200x800;
      gRenderer().updateRendererConfigs(&params);
      gEngine().getWindow()->setToWindowed(1200, 800); 
      gEngine().getWindow()->setToCenter();
      gEngine().getWindow()->show(); 
    }

    // Testing renderer configurations during runtime.
    if (Keyboard::keyPressed(KEY_CODE_8)) {
      GraphicsConfigParams params = gRenderer().getCurrentGraphicsConfigs();
      params._Buffering = SINGLE_BUFFER;
      params._EnableVsync = true;
      params._EnableBloom = false;
      params._AA = AA_None;
      params._Shadows = GRAPHICS_QUALITY_NONE;
      params._TextureQuality = GRAPHICS_QUALITY_ULTRA;
      params._Lod = 5.0f;
      params._EnableMultithreadedRendering = true;
      gRenderer().updateRendererConfigs(&params);
    }

    if (Keyboard::keyPressed(KEY_CODE_9)) {
      GraphicsConfigParams params = gRenderer().getCurrentGraphicsConfigs();
      params._Buffering = TRIPLE_BUFFER;
      params._EnableVsync = false;
      params._EnableBloom = true;
      params._EnableMultithreadedRendering = false;
      params._AA = AA_FXAA_2x;
      params._Lod = 0.0f;
      params._Shadows = GRAPHICS_QUALITY_HIGH;
      params._TextureQuality = GRAPHICS_QUALITY_ULTRA;
      gRenderer().updateRendererConfigs(&params);
    }

    if ( Keyboard::keyPressed( KEY_CODE_2 ) )
    {
      m_pause = !m_pause;
      Log() << "Pressed\n";
      if (m_pause)
      {
        m_pX = Mouse::getX( );
        m_pY = Mouse::getY( );
        Mouse::setEnable(true);
        Mouse::show(true);
      }
      else
      {
        Mouse::setEnable(false);
        Mouse::setPosition( m_pX, m_pY );
        Mouse::show(false);
      }
    }

    if (bFirstLook) {
      m_lastX = (R32)Mouse::getX();
      m_lastY = (R32)Mouse::getY();
      bFirstLook = false;
    }

    if ( !m_pause ) {
      R32 xoffset = m_lastX - (R32)Mouse::getX();
      R32 yoffset = (R32)Mouse::getY() - m_lastY;
      m_lastX = (R32)Mouse::getX();
      m_lastY = (R32)Mouse::getY();

      xoffset *= m_xSensitivity;
      yoffset *= m_ySensitivity;

      m_yaw += xoffset;
      m_pitch += yoffset;
      if (m_pitch < -Radians(269.5f)) {
        m_pitch = -Radians(269.5f);
      }
      if (m_pitch > -Radians(90.5f)) {
        m_pitch = -Radians(90.5f);
      }
    }

    Vector3 euler = Vector3(m_pitch, m_yaw, m_roll);
    transform->_rotation = Quaternion::eulerAnglesToQuaternion(euler);

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
        m_temp.getTransform()->_position = hitOut._worldHit;
        m_temp.start();

        GameObject* obj = hitOut._rigidbody->_gameObj;
        Item* item = obj->castTo<Item>();
        if (item) {
          _pHolding = item;
          item->GetPhysicsComponent()->setMass(0.0f);
        }
      }

    }

    m_temp.update(tick);

    if (Keyboard::keyPressed(KEY_CODE_E) && _pHolding) {
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
    m_pSpotLight->cleanUp();
    m_pSpotLight2->cleanUp();

    delete m_pSpotLight;
    delete m_pSpotLight2;
  }

private:
  Camera * pCam;
  B8      bFirstLook;
  R32     m_lastX;
  R32     m_lastY;
  R32     m_xSensitivity;
  R32     m_ySensitivity;
  R32     m_pitch;
  R32     m_yaw;
  R32     m_roll;
  R32     m_constrainPitch;
  B32     m_pause;
  R32     m_pX, m_pY;
#if CAMERA_REVOLVE > 0
  R32     t = 0.0f;
#endif
  SpotLightComponent* m_pSpotLight;
  SpotLightComponent* m_pSpotLight2;
  TempExplosion   m_temp;
  B32               bFollow;
  // Object to hold on to.
  Item*             _pHolding;
  Transform         oldTransform;
};



class Mover : public GameObject {
  R_GAME_OBJECT(Mover);
public:
  MainCamera* pMainCam;

  Mover()
    : m_speed(5.0f)
    , pMainCam(nullptr)
    , m_camOffset(1.0f)
    , offset(1.0f)
    , m_jumping(false) { }

  void onStartUp() {
    // Anything in contact with this flying camera will get pushed aside.
    m_pCollider = gPhysics().createBoxCollider(Vector3(1.0f, 1.0f, 1.0f));
    m_pPhysicsComponent = new PhysicsComponent();
    m_pPhysicsComponent->initialize(this);
    m_pPhysicsComponent->addCollider(m_pCollider);
    m_pPhysicsComponent->setAngleFactor(Vector3(0.0f, 1.0f, 0.0f));
    m_pPhysicsComponent->setFriction(2.0f);

    bFollow = false;
    m_pPhysicsComponent->setMass(1.0f);
  }

  void update(R32 tick) override {
    Transform* transform = getTransform();
    R32 speed = m_speed;
    Vector3 f = transform->front();
    Vector3 r = transform->right();

    // Use main camera's rotations, based on where user is looking.
    if (pMainCam) {
      Transform* camTransform = pMainCam->getTransform();
      camTransform->_position = transform->_position;
      if ( Keyboard::keyPressed( KEY_CODE_LCONTROL ) ||
           Keyboard::keyHeldDown( KEY_CODE_LCONTROL ) ) {
        offset = Lerpf(offset, 0.0f, tick * 5.0f);
        speed *= 0.5f;
      } else {
        offset = Lerpf(offset, m_camOffset, tick * 9.0f);
      }

      camTransform->_position.y += offset;

      f = camTransform->front();
      r = camTransform->right();
    }

    f.y = 0.0f;
    f = f.normalize();

    if (Keyboard::keyPressed(KEY_CODE_LSHIFT)) {
      speed *= 5.0f;
    }
    if (Keyboard::keyPressed( KEY_CODE_A ) ||
        Keyboard::keyHeldDown( KEY_CODE_A ) ) {
      transform->_position -= r * speed * tick;
    }

    if ( Keyboard::keyPressed( KEY_CODE_D ) ||
         Keyboard::keyHeldDown( KEY_CODE_D ) ) {
      transform->_position += r * speed * tick;
    }

    if ( Keyboard::keyPressed( KEY_CODE_W ) ||
         Keyboard::keyHeldDown( KEY_CODE_W ) ) {
      transform->_position += f * speed * tick;
    }

    if ( Keyboard::keyPressed( KEY_CODE_S ) ||
         Keyboard::keyHeldDown( KEY_CODE_S ) ) {
      transform->_position -= f * speed * tick;
    }

    if (Keyboard::keyPressed(KEY_CODE_SPACE)) {
      if (!m_jumping) {
        m_pPhysicsComponent->setLinearFactor(Vector3(1.0f, 1.0f, 1.0f));
        m_pPhysicsComponent->applyImpulse(Vector3(0.0f, 10.0f, 0.0f),
                                          Vector3());
        m_jumping = true;
      }
    }
  }


  void onCleanUp() override {
    m_pPhysicsComponent->cleanUp();
    delete m_pPhysicsComponent;
    delete m_pCollider;
  }


  void onCollisionEnter(Collision* other) override {
    CubeObject* cube = other->_gameObject->castTo<CubeObject>();
    if (cube) {
      m_pPhysicsComponent->setLinearFactor(Vector3(1.0, 0.0f, 1.0f));
      m_pPhysicsComponent->setLinearVelocity(Vector3());
      m_pPhysicsComponent->clearForces();
      m_jumping = false;
      Log() << "enter CubeObject\n";
    }
  }

  void onCollisionExit(Collision* other) override {
    CubeObject* cube = other->_gameObject->castTo<CubeObject>();
    if (cube) {
      m_pPhysicsComponent->setLinearFactor(Vector3(1.0, 1.0f, 1.0f));
      // m_pPhysicsComponent->setLinearVelocity(Vector3());
      // m_pPhysicsComponent->clearForces();
      Log() << "exit CubeObject\n";
    } else {
       //m_pPhysicsComponent->clearForces();
       m_pPhysicsComponent->setLinearVelocity(Vector3(0.0f, m_pPhysicsComponent->getRigidBody()->_velocity.y, 0.0f));
      // Log() << "Stop.\n";
    }
  }

  void onCollisionStay(Collision* other) override {
    // Log() << "Collision stay\n";
    //m_pPhysicsComponent->setLinearVelocity(
    //    Vector3(0.0f /*m_pPhysicsComponent->getRigidBody()->_velocity.x*/, 
    //            0.0f,
    //            0.0f /*m_pPhysicsComponent->getRigidBody()->_velocity.z*/));
    if (other->_gameObject->castTo<CubeObject>()) return;

    if (m_jumping) {
      for (U32 i = 0; i < other->_contactPoints.size(); ++i) {
        if ((Vector3::UP.dot(other->_contactPoints[i]._normal) >=
             1.0f - 0.3f) &&
            (Vector3::UP.dot(other->_contactPoints[i]._normal) <=
             1.0f + 0.3f)) {
          Log() << "enter stop jumping.\n";
          Log() << other->_contactPoints[i]._normal << "\n";
          m_pPhysicsComponent->setLinearFactor(Vector3(1.0f, 1.0f, 1.0f));
          m_jumping = false;
          break;
        }
      }
    }
  }

private:
  PhysicsComponent* m_pPhysicsComponent;
  Collider* m_pCollider;
  B8 m_jumping;
  B8 bFollow;
  R32 m_camOffset;
  R32 offset;
  R32 m_speed;
};