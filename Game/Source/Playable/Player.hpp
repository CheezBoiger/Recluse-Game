// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "IActor.hpp"

#include "../Camera/PlayerCamera.hpp"

#include <vector>
#include <unordered_map>

class IITem;

class IInventory {
  std::vector<IItem*>                     _items;
  std::unordered_map<std::string, IITem*> _itemMap;
};


// Player script. User controls are managed by this class.
class Player : public IActor {
  R_GAME_OBJECT(Player)
public:

  virtual void onStartUp() override
  {
    m_playerCamera = new PlayerCamera();
    if (m_playerCamera) {
      m_playerCamera->SetTargetActor(this);
    }
  }

  virtual void update(R32 tick) override
  {
    Transform* transform = getTransform();

    R32 speed = m_currentMoveSpeed;

    if (Keyboard::KeyPressed(m_sprintKey)) {
      speed *= m_sprintFactor;
    }

    if (Keyboard::KeyPressed(m_moveForward)) {
      transform->_position += transform->front() * speed * tick;
    }

    if (Keyboard::KeyPressed(m_moveBackward)) {
      transform->_position -= transform->front() * speed * tick;
    }

    if (Keyboard::KeyPressed(m_strafeLeft)) {
      transform->_position -= transform->right() * speed * tick;
    }

    if (Keyboard::KeyPressed(m_strafeRight)) {
      transform->_position += transform->right() * speed * tick;
    }

    if (Keyboard::KeyPressed(m_jump) && !(m_activity & Activity_Jumping)) {
      m_activity |= Activity_Jumping;
      m_physicsComponent.applyImpulse(transform->up() * m_jumpStrength, Vector3());
    }


    if (Mouse::buttonDown(Mouse::LEFT)) {
      RayTestHit hitOut;
      if (gPhysics().rayTest(transform->_position, m_playerCamera->GetFront(), 50.0f, &hitOut)) {
        GameObject* obj = hitOut._rigidbody->_gameObj;
        IActor* actor = obj->castTo<IActor>();
        if (actor) {
          
        }
      }
    }


    m_playerCamera->UpdateCamera();
  }


  void SetCamera(Camera* camera) 
  {
  }


  virtual void onCleanUp() override
  {
    delete m_playerCamera;
    m_playerCamera = nullptr;
  }

  PlayerCamera* GetPlayerCamera()
  {
    return m_playerCamera;
  }
  
  virtual void onCollision(Collision* collision) override
  {
  }

private:

  PlayerCamera* m_playerCamera;
  IInventory*   m_pInventory;
  KeyCode       m_moveForward;
  KeyCode       m_moveBackward;
  KeyCode       m_jump;
  KeyCode       m_strafeLeft;
  KeyCode       m_strafeRight;
  KeyCode       m_sprintKey;
};