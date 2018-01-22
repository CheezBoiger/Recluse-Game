// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"
#include "Collision.hpp"

#include "Core/Types.hpp"
#include "PxPhysicsAPI.h"
#include "PxPhysics.h"

#include "foundation/PxMath.h"

#include "cudamanager/PxCudaContextManager.h"
#include "cudamanager/PxCudaMemoryManager.h"

using namespace physx;

namespace Recluse {


class PhysXActor {
public:
  virtual         ~PhysXActor() { }

  PxActor*        mActor;
};


class PhysXCollider : public Collider {
public:
  virtual         ~PhysXCollider() { }

  void            AssignToActor(Actor* actor) override { }
  void            CleanUp() override { }
  void            Initialize() override { }

  PxShape*        mShape;
  PxActor*        mActor;
};


class PhysXBoxCollider : public PhysXCollider {
public:
};


class PhysXSphereCollider : public PhysXCollider {
public:
};


class PhysX {
public:
  PhysX()
    : m_pPhysics(nullptr)
    , m_pFoundation(nullptr)
    , m_pPhysicsScene(nullptr) { }

  b8                        Initialize();
  void                      CleanUp();
  void                      Update(r32 stepTime);

  void                      SetScene(PxScene* pScene) { m_pPhysicsScene = pScene; }

  PxScene*                  CreateScene(PxVec3 gravity, PxSimulationFilterShader& filter,
                                        PxSceneFlags flags, u32 threads, b8 useGpu = false);

private:
  PxPhysics*                m_pPhysics;
  PxFoundation*             m_pFoundation;
  PxDefaultErrorCallback    m_DefaultErrorCallback;
  PxDefaultAllocator        m_DefaultAllocatorCallback;
  PxTolerancesScale         m_Tolerance;
  PxScene*                  m_pPhysicsScene;
};
} // Recluse