// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Collider.hpp"
#include "Collision.hpp"

#include "Core/Types.hpp"
#include "PxPhysicsAPI.h"
#include "PxPhysics.h"

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
    : mPhysics(nullptr)
    , mFoundation(nullptr) { }

  b8                              Initialize();
  void                            CleanUp();


  physx::PxPhysics*               mPhysics;
  physx::PxFoundation*            mFoundation;
  physx::PxDefaultErrorCallback   mDefaultErrorCallback;
  physx::PxDefaultAllocator       mDefaultAllocatorCallback;
  physx::PxTolerancesScale        mTolerance;
};
} // Recluse