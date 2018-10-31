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

  virtual void OnStartUp() override
  {
    m_playerCamera = new PlayerCamera();
    if (m_playerCamera) {
      m_playerCamera->SetTargetActor(this);
    }
  }

  virtual void Update(r32 tick) override
  {
    Transform* transform = GetTransform();

    r32 speed = m_currentMoveSpeed;

    if (Keyboard::KeyPressed(m_sprintKey)) {
      speed *= m_sprintFactor;
    }

    if (Keyboard::KeyPressed(m_moveForward)) {
      transform->Position += transform->Front() * speed * tick;
    }

    if (Keyboard::KeyPressed(m_moveBackward)) {
      transform->Position -= transform->Front() * speed * tick;
    }

    if (Keyboard::KeyPressed(m_strafeLeft)) {
      transform->Position -= transform->Right() * speed * tick;
    }

    if (Keyboard::KeyPressed(m_strafeRight)) {
      transform->Position += transform->Right() * speed * tick;
    }

    if (Keyboard::KeyPressed(m_jump) && !(m_activity & Activity_Jumping)) {
      m_activity |= Activity_Jumping;
      m_physicsComponent.ApplyImpulse(transform->Up() * m_jumpStrength, Vector3());
    }


    m_playerCamera->UpdateCamera();
  }


  void SetCamera(Camera* camera) 
  {
  }


  virtual void OnCleanUp() override
  {
    delete m_playerCamera;
    m_playerCamera = nullptr;
  }

  PlayerCamera* GetPlayerCamera()
  {
    return m_playerCamera;
  }
  
  virtual void OnCollision(Collision* collision) override
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