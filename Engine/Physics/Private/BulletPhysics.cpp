// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "BulletPhysics.hpp"
#include "Core/Exception.hpp"
#include "Core/Utility/Time.hpp"
#include "Core/Exception.hpp"

#include "Collider.hpp"
#include "BoxCollider.hpp"
#include "RigidBody.hpp"

#include <unordered_map>

namespace Recluse {


struct RigidBundle {
  RigidBody*          rigidBody;
  btRigidBody*        native;
  btMotionState*      motionState;
};

std::unordered_map<uuid64, RigidBundle> kRigidBodyMap;
std::unordered_map<Collider*, btCollisionShape*> kCollisionShapes;

// Global physics manager that holds physics contraint solvers, dispatchers, configuration
// and management.
struct bt_physics_manager 
{
  // 
  btDefaultCollisionConfiguration*      _pCollisionConfiguration;

  // 
  btCollisionDispatcher*                _pDispatcher;


  btBroadphaseInterface*                _pOverlappingPairCache;

  // Interchangeable constraint solver. We will use Sequential Impulse, as it is
  // popular. We can use Projected Gauss-Seidel for experimentation later... 
  btSequentialImpulseConstraintSolver*  _pSolver;

  btDiscreteDynamicsWorld*              _pWorld;
} bt_manager;


RigidBody* GetRigidBody(uuid64 key)
{
  RigidBody* body = nullptr;

  if (kRigidBodyMap.find(key) != kRigidBodyMap.end()) {
    body = kRigidBodyMap[key].rigidBody;  
  }

  return body;
}


btCollisionShape* GetCollisionShape(Collider* shape)
{
  btCollisionShape* pShape = nullptr;
  if (kCollisionShapes.find(shape) != kCollisionShapes.end()) {
    pShape = kCollisionShapes[shape];
  }
  return pShape;
}


BulletPhysics& gBulletEngine()
{
  static BulletPhysics bullet;
  return bullet;
}


void BulletPhysics::Initialize()
{
  bt_manager._pCollisionConfiguration = new btDefaultCollisionConfiguration();
  bt_manager._pDispatcher = new btCollisionDispatcher(bt_manager._pCollisionConfiguration);
  bt_manager._pOverlappingPairCache = new btDbvtBroadphase();
  bt_manager._pSolver = new btSequentialImpulseConstraintSolver();

  bt_manager._pWorld = new btDiscreteDynamicsWorld(bt_manager._pDispatcher, 
    bt_manager._pOverlappingPairCache, bt_manager._pSolver, 
    bt_manager._pCollisionConfiguration
  );

  // Default gravity of the world.
  bt_manager._pWorld->setGravity(btVector3(0.0f, -10.0f, 0.0f));
  R_DEBUG(rNotify, "Bullet Sdk initialized.\n");

  
}


void BulletPhysics::CleanUp()
{
  for (auto& it : kCollisionShapes) {
    delete it.second;
  }

  for (auto& it : kRigidBodyMap) {
    btCollisionObject* obj = it.second.native;
    bt_manager._pWorld->removeCollisionObject(obj);
    
    delete it.second.motionState;
    delete it.second.native;
    delete it.second.rigidBody;
  }
  delete bt_manager._pWorld;
  delete bt_manager._pSolver;
  delete bt_manager._pOverlappingPairCache;
  delete bt_manager._pDispatcher;
  delete bt_manager._pCollisionConfiguration;
  R_DEBUG(rNotify, "Bullet Sdk cleaned up.\n");
}


RigidBody* BulletPhysics::CreateRigidBody(const Vector3& centerOfMassOffset, Collider* shape)
{
  RigidBody* rigidbody = new RigidBody();
  btCollisionShape* pShape = GetCollisionShape(shape);
  btDefaultMotionState* pMotionState = new btDefaultMotionState(
    btTransform(btQuaternion(0.f, 0.f, 0.f, 1.f), 
      btVector3(centerOfMassOffset.x, centerOfMassOffset.y, centerOfMassOffset.z)
    )
  );

  btRigidBody::btRigidBodyConstructionInfo info(
    1.f,
    pMotionState,
    pShape,
    btVector3(0.f, 0.f, 0.f)
  );

  btRigidBody* pNativeBody = new btRigidBody(info);
  pNativeBody->setUserPointer(rigidbody);
  RigidBundle bundle = { rigidbody, pNativeBody, pMotionState };
  // Store body into map.
  kRigidBodyMap[rigidbody->GetUUID()] = bundle;
  // And store to world.
  bt_manager._pWorld->addRigidBody(pNativeBody);
  
  return rigidbody;
}


Collider* BulletPhysics::CreateBoxCollider(const Vector3& scale)
{
  Collider* collider = new BoxCollider();
  btCollisionShape* pShape = new btBoxShape(
    btVector3(btScalar(scale.x), btScalar(scale.y), btScalar(scale.z)));
  kCollisionShapes[collider] = pShape;
  return collider;
}


void BulletPhysics::FreeRigidBody(RigidBody* body)
{
  if (!body) return;
  R_DEBUG(rVerbose, "Freeing rigid body.\n");
  uuid64 uuid = body->GetUUID();
  RigidBundle& bundle = kRigidBodyMap[uuid];
  
  bt_manager._pWorld->removeRigidBody(bundle.native);
  
  delete bundle.rigidBody;
  delete bundle.native->getMotionState();
  delete bundle.native;

  kRigidBodyMap.erase(uuid);
}


void BulletPhysics::Update(r64 dt)
{
  // TODO(): Needs assert.
  if (!bt_manager._pWorld) { return; }

  bt_manager._pWorld->stepSimulation(btScalar(dt), 10, btScalar(Time::FixTime));
  btDispatcher* pDispatcher = bt_manager._pWorld->getDispatcher();
  u32 numManifolds = pDispatcher->getNumManifolds();
  for (u32 i = 0; i < numManifolds; ++i) {
    btPersistentManifold* contactManifold = pDispatcher->getManifoldByIndexInternal(i);
    const btCollisionObject* objA = contactManifold->getBody0();
    const btCollisionObject* objB = contactManifold->getBody1();

    u32 numContacts = contactManifold->getNumContacts();
    for (u32 j = 0; j < numContacts; ++j) {
      btManifoldPoint& pt = contactManifold->getContactPoint(j);
      if (pt.getDistance() < 0.0f) {
        const btVector3& ptA = pt.getPositionWorldOnA();
        const btVector3& ptB = pt.getPositionWorldOnB();
        const btVector3& normalOnB = pt.m_normalWorldOnB;
        // Collision here.
        RigidBody* bodyA = GetRigidBody(static_cast<RigidBody*>(objA->getUserPointer())->GetUUID());
        RigidBody* bodyB = GetRigidBody(static_cast<RigidBody*>(objB->getUserPointer())->GetUUID());
        
        if (bodyA && bodyA->onCollisionCallback) bodyA->onCollisionCallback();
        if (bodyB && bodyB->onCollisionCallback) bodyB->onCollisionCallback();
      }  
    }
  }

  // Copy data from physics sim.
  for (auto& it : kRigidBodyMap) {
    R_ASSERT(it.second.native, "Bullet RigidBody was null");
    R_ASSERT(it.second.rigidBody, "Engine RigidBody was null.");
    RigidBundle& bundle = it.second;
    btTransform transform = bundle.native->getWorldTransform();
    btQuaternion q = transform.getRotation();
    btVector3 p = transform.getOrigin();

    bundle.rigidBody->m_qRotation = Quaternion(q.x(), q.y(), q.z(), q.w());
    bundle.rigidBody->m_vPosition = Vector3(p.x(), p.y(), p.z());
  }
}


void BulletPhysics::SetMass(RigidBody* body, r32 mass)
{
  if (!body) return;
  uuid64 key = body->GetUUID();
  btRigidBody* obj = kRigidBodyMap[key].native;
  bt_manager._pWorld->removeRigidBody(obj);
  btVector3 inertia;
  obj->setMassProps(btScalar(mass), inertia);
  bt_manager._pWorld->addRigidBody(obj);
}


void BulletPhysics::SetTransform(RigidBody* body, const Vector3& newPos, const Quaternion& newRot)
{
  if (!body) return;
  uuid64 key = body->GetUUID();
  btRigidBody* obj = kRigidBodyMap[key].native;
  btTransform transform = obj->getWorldTransform();
  transform.setOrigin(btVector3(newPos.x, newPos.y, newPos.z));
  transform.setRotation(btQuaternion(
    btScalar(newRot.x),
    btScalar(newRot.y),
    btScalar(newRot.z),
    btScalar(newRot.w)));

  // TODO(): We shouldn't always have to update the transforms, should check if object is 
  // static or not moving in physics...
  obj->setWorldTransform(transform);
  obj->activate();
}


void BulletPhysics::ActivateRigidBody(RigidBody* body)
{
  if (!body) return;
  uuid64 key = body->GetUUID();
  btRigidBody* rb = kRigidBodyMap[key].native;
  rb->activate();
}


void BulletPhysics::DeactivateRigidBody(RigidBody* body)
{
  if (!body) return;
  uuid64 key = body->GetUUID();
  btRigidBody* rb = kRigidBodyMap[key].native;
  rb->setActivationState(WANTS_DEACTIVATION);
}


void BulletPhysics::SetWorldGravity(const Vector3& gravity)
{
  bt_manager._pWorld->setGravity(btVector3(
    btScalar(gravity.x),
    btScalar(gravity.y),
    btScalar(gravity.z))
  );
}
} // Recluse