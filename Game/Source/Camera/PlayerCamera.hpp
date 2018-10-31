// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Game/Engine.hpp"
#include "Game/Camera.hpp"

using namespace Recluse;

class IActor;
class PlayerActor;


// Camera used for player to look. First Person Camera.
class PlayerCamera {
public:

  void SetTargetActor(IActor* actor) { m_actorTarget = actor; }
  void Initialize(GameObject* obj) { 
    m_pCamera->Initialize(obj);
    m_lastX = (r32)Mouse::X();
    m_lastY = (r32)Mouse::Y();
  }

  void UpdateCamera()
  {
    Transform* transform = m_pCamera->GetTransform();
    Quaternion rot = transform->Rotation;
    Vector3 euler = rot.ToEulerAngles();
    m_yaw = euler.x;
    m_pitch = euler.y;

    r32 lx = (r32)Mouse::X();
    r32 ly = (r32)Mouse::Y();  

    r32 xoffset = m_lastX - lx;
    r32 yoffset = ly - m_lastY;
    m_lastX = lx;
    m_lastY = ly;

    xoffset *= m_sensitivityX;
    yoffset *= m_sensitivityY;

    m_yaw += xoffset;
    m_pitch += yoffset;

    m_pCamera->Update();
  }


  void SetThisCamera()
  {
    Camera::SetMain(m_pCamera);
  }

  Vector3 GetFront() const
  {
    return m_pCamera->GetTransform()->Front();
  }

  Vector3 GetPosition() const 
  {
    return m_pCamera->GetTransform()->Position;
  }

private:
  // Actor that this player camera looks through.
  IActor* m_actorTarget;

  // Camera object used by the engine.
  Camera* m_pCamera;

  r32     m_lastX;
  r32     m_lastY;
  r32     m_yaw;
  r32     m_pitch;
  r32     m_sensitivityX;
  r32     m_sensitivityY;
};