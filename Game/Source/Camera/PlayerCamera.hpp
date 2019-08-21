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
  void initialize(GameObject* obj) { 
    m_pCamera->initialize(obj);
    m_lastX = (R32)Mouse::getX();
    m_lastY = (R32)Mouse::getY();
  }

  void UpdateCamera()
  {
    Transform* transform = m_pCamera->getTransform();
    Quaternion rot = transform->_rotation;
    Vector3 euler = rot.toEulerAngles();
    m_yaw = euler.x;
    m_pitch = euler.y;

    R32 lx = (R32)Mouse::getX();
    R32 ly = (R32)Mouse::getY();  

    R32 xoffset = m_lastX - lx;
    R32 yoffset = ly - m_lastY;
    m_lastX = lx;
    m_lastY = ly;

    xoffset *= m_sensitivityX;
    yoffset *= m_sensitivityY;

    m_yaw += xoffset;
    m_pitch += yoffset;
    if (m_pitch < -Radians(270.0f)) {
      m_pitch = -Radians(270.0f);
    }
    if (m_pitch > -Radians(90.0f)) {
      m_pitch = -Radians(90.0f);
    }

    m_pCamera->update();
  }


  void SetThisCamera()
  {
    Camera::setMain(m_pCamera);
  }

  Vector3 GetFront() const
  {
    return m_pCamera->getTransform()->front();
  }

  Vector3 getPosition() const 
  {
    return m_pCamera->getTransform()->_position;
  }

private:
  // Actor that this player camera looks through.
  IActor* m_actorTarget;

  // Camera object used by the engine.
  Camera* m_pCamera;

  R32     m_lastX;
  R32     m_lastY;
  R32     m_yaw;
  R32     m_pitch;
  R32     m_sensitivityX;
  R32     m_sensitivityY;
};