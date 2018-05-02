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


// Main camera is an object in the scene.
class MainCamera : public GameObject
{
public:
  MainCamera()
    : m_xSensitivity(0.0025f)
    , m_ySensitivity(0.0025f)
    , m_yaw(0.0f)
    , m_pitch(0.0f)
    , bFirstLook(true)
    , m_constrainPitch(Radians(90.0f))
    , m_lastX(0.0f)
    , m_lastY(0.0f)
    , m_speed(5.0f)
  {
    Window* pWindow = gEngine().GetWindow();
    Transform* transform = GetTransform();
    // Camera set.
    pCam = new Camera(Camera::PERSPECTIVE, Radians(60.0f),
      static_cast<r32>(pWindow->Width()),
      static_cast<r32>(pWindow->Height()),
      0.001f, 2000.0f);
    pCam->Initialize(this);
    pCam->EnableBloom(true);
    Camera::SetMain(pCam);

    transform->Position = Vector3(10.0f, 10.0f, 10.0f);
    Vector3 dir = Vector3(0.0f, 0.0f, 0.0f) - transform->Position;
    transform->Rotation = Quaternion::LookRotation(dir, Vector3::UP);
    Vector3 euler = transform->Rotation.ToEulerAngles();
    m_pitch = euler.x;
    m_yaw = euler.y;
    m_roll = euler.z;
  }

  void Update(r32 tick) override
  {
    if (Keyboard::KeyPressed(KEY_CODE_ESCAPE)) { gEngine().SignalStop(); }
    Transform* transform = GetTransform();

    if (Keyboard::KeyPressed(KEY_CODE_A)) { transform->Position -= transform->Right() * m_speed * tick; }
    if (Keyboard::KeyPressed(KEY_CODE_D)) { transform->Position += transform->Right() * m_speed * tick; }
    if (Keyboard::KeyPressed(KEY_CODE_W)) { transform->Position += transform->Forward() * m_speed * tick; }
    if (Keyboard::KeyPressed(KEY_CODE_S)) { transform->Position -= transform->Forward() * m_speed * tick; }
    if (Keyboard::KeyPressed(KEY_CODE_N)) { Time::ScaleTime -= 4.0 * Time::DeltaTime; }
    if (Keyboard::KeyPressed(KEY_CODE_M)) { Time::ScaleTime += 4.0 * Time::DeltaTime; }

    if (bFirstLook) {
      m_lastX = (r32)Mouse::X();
      m_lastY = (r32)Mouse::Y();
      bFirstLook = false;
    }

    r32 xoffset = m_lastX - (r32)Mouse::X();
    r32 yoffset = (r32)Mouse::Y() - m_lastY;
    m_lastX = (r32)Mouse::X();
    m_lastY = (r32)Mouse::Y();

    xoffset *= m_xSensitivity;
    yoffset *= m_ySensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    Vector3 euler = Vector3(m_pitch, m_yaw, m_roll);
    transform->Rotation = Quaternion::EulerAnglesToQuaternion(euler);
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
};