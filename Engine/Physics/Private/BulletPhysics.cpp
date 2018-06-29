// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "BulletPhysics.hpp"
#include "Core/Exception.hpp"
#include "Core/Utility/Time.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"

#include "Renderer/Renderer.hpp"
#include "Collider.hpp"
#include "BoxCollider.hpp"
#include "SphereCollider.hpp"
#include "Collision.hpp"
#include "RigidBody.hpp"

#include <unordered_map>

namespace Recluse {


struct RigidBundle {
  RigidBody*          rigidBody;
  btRigidBody*        native;
  btCompoundShape*    compound;
};

std::unordered_map<physics_uuid_t, RigidBundle> kRigidBodyMap;
std::unordered_map<physics_uuid_t, btCollisionShape*> kCollisionShapes;

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
  if (kCollisionShapes.find(shape->GetUUID()) != kCollisionShapes.end()) {
    pShape = kCollisionShapes[shape->GetUUID()];
  }
  return pShape;
}


void BulletPhysics::OnStartUp()
{
  if (!gRenderer().IsActive()) {
    R_DEBUG(rWarning, "Renderer is not active! Physics will carry on however...\n");
  }

  Initialize();

  R_DEBUG(rNotify, "Physics Engine is successfully initialized.\n");
}


void BulletPhysics::OnShutDown()
{
  CleanUp();
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
    
    delete it.second.native->getMotionState();
    delete it.second.native->getCollisionShape();
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


RigidBody* BulletPhysics::CreateRigidBody(const Vector3& centerOfMassOffset)
{
  btCompoundShape* compound = new btCompoundShape();
  RigidBody* rigidbody = new RigidBody();
  btDefaultMotionState* pMotionState = new btDefaultMotionState(
    btTransform(btQuaternion(0.f, 0.f, 0.f, 1.f), 
      btVector3(centerOfMassOffset.x, centerOfMassOffset.y, centerOfMassOffset.z)
    )
  );

  btVector3 localInertia(0, 0, 0);
  compound->calculateLocalInertia(1.0f, localInertia);

  btRigidBody::btRigidBodyConstructionInfo info(
    btScalar(rigidbody->m_mass),
    pMotionState,
    compound,
    localInertia
  );

  btRigidBody* pNativeBody = new btRigidBody(info);
  pNativeBody->setUserPointer(rigidbody);
  RigidBundle bundle = { rigidbody, pNativeBody, compound };
  // Store body into map.
  kRigidBodyMap[rigidbody->GetUUID()] = bundle;
  // And store to world.
  bt_manager._pWorld->addRigidBody(pNativeBody);
  
  return rigidbody;
}


BoxCollider* BulletPhysics::CreateBoxCollider(const Vector3& scale)
{
  BoxCollider* collider = new BoxCollider();
  btCollisionShape* pShape = new btBoxShape(
    btVector3(btScalar(scale.x), btScalar(scale.y), btScalar(scale.z)));
  kCollisionShapes[collider->GetUUID()] = pShape;
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


void BulletPhysics::UpdateState(r64 dt, r64 tick)
{
  // TODO(): Needs assert.
  if (!bt_manager._pWorld) { return; }
  bt_manager._pWorld->stepSimulation(btScalar(dt), 10, btScalar(tick));
  btDispatcher* pDispatcher = bt_manager._pWorld->getDispatcher();
  u32 numManifolds = pDispatcher->getNumManifolds();

  for (u32 i = 0; i < numManifolds; ++i) {
    btPersistentManifold* contactManifold = pDispatcher->getManifoldByIndexInternal(i);
    const btCollisionObject* objA = contactManifold->getBody0();
    const btCollisionObject* objB = contactManifold->getBody1();

    Collision collisionOnA;
    Collision collisionOnB;

    u32 numContacts = contactManifold->getNumContacts();
    collisionOnA._contactPoints.resize(numContacts);
    collisionOnB._contactPoints.resize(numContacts);
    for (u32 j = 0; j < numContacts; ++j) {
      btManifoldPoint& pt = contactManifold->getContactPoint(j);
      if (pt.getDistance() < 0.0f) {
        const btVector3& ptA = pt.getPositionWorldOnA();
        const btVector3& ptB = pt.getPositionWorldOnB();
        const btVector3& normalOnB = pt.m_normalWorldOnB;

        ContactPoint& contactA = collisionOnA._contactPoints[j];
        ContactPoint& contactB = collisionOnB._contactPoints[j];

        contactA._point = Vector3(ptB.x(), ptB.y(), ptB.z());
        contactA._distance = pt.getDistance();

        contactB._point = Vector3(ptA.x(), ptA.y(), ptA.z());
        contactB._distance = pt.getDistance();
      }  
    }

    // Collision here.
    RigidBody* bodyA = GetRigidBody(static_cast<RigidBody*>(objA->getUserPointer())->GetUUID());
    RigidBody* bodyB = GetRigidBody(static_cast<RigidBody*>(objB->getUserPointer())->GetUUID());

    collisionOnA._gameObject = bodyB->GetGameObject();
    collisionOnA._rigidBody = bodyB;

    collisionOnB._gameObject = bodyA->GetGameObject();
    collisionOnB._rigidBody = bodyA;

    bodyA->InvokeCollision(&collisionOnA);
    bodyB->InvokeCollision(&collisionOnB);
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
  btCollisionShape* shape = obj->getCollisionShape();
  shape->calculateLocalInertia(btScalar(mass), inertia);
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
  if (body->Activated()) obj->activate();
  
}


void BulletPhysics::ActivateRigidBody(RigidBody* body)
{
  if (!body) return;
  uuid64 key = body->GetUUID();
  btRigidBody* rb = kRigidBodyMap[key].native;
  body->m_bActivated = true;
  rb->activate();
}


void BulletPhysics::DeactivateRigidBody(RigidBody* body)
{
  if (!body) return;
  uuid64 key = body->GetUUID();
  btRigidBody* rb = kRigidBodyMap[key].native;
  body->m_bActivated = false;
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


void BulletPhysics::ApplyImpulse(RigidBody* body, const Vector3& impulse, const Vector3& relPos)
{
  R_ASSERT(body, "Rigid Body was null.");
  uuid64 k = body->GetUUID();
  btRigidBody* rb = kRigidBodyMap[k].native;
  rb->applyImpulse(btVector3(btScalar(impulse.x), btScalar(impulse.y), btScalar(impulse.z)), 
    btVector3(btScalar(relPos.x), btScalar(relPos.y), btScalar(relPos.z)));
}


b32 BulletPhysics::RayTest(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHit* output)
{
  btVector3 start(btScalar(origin.x),
    btScalar(origin.y),
    btScalar(origin.z));
  btVector3 dir(btScalar(direction.x),
    btScalar(direction.y),
    btScalar(direction.z));
  dir.normalize();

  btVector3 end = start + dir * maxDistance;

  btCollisionWorld::ClosestRayResultCallback hit(start, end);
  bt_manager._pWorld->rayTest(start, end, hit);
 
  // Register hit.
  if (!hit.hasHit()) return false; 
  RigidBody* rbHit = static_cast<RigidBody*>(hit.m_collisionObject->getUserPointer());
  output->_rigidbody = rbHit;
  output->_collider = rbHit->GetCollider();
  output->_normal = Vector3(hit.m_hitNormalWorld.x(),
                            hit.m_hitNormalWorld.y(),
                            hit.m_hitNormalWorld.z());
  hit.m_closestHitFraction;
  return true;
}


b32 BulletPhysics::RayTestAll(const Vector3& origin, const Vector3& direction, const r32 maxDistance, RayTestHitAll* output)
{
  btVector3 start(btScalar(origin.x),
    btScalar(origin.y),
    btScalar(origin.z));
  btVector3 dir(btScalar(direction.x),
    btScalar(direction.y),
    btScalar(direction.z));
  dir.normalize();

  btVector3 end = start + dir * maxDistance;
  btCollisionWorld::AllHitsRayResultCallback allHits(start, end);
  bt_manager._pWorld->rayTest(start, end, allHits);

  // Register hits.
  if (!allHits.hasHit()) return false;
  output->_colliders.resize(allHits.m_collisionObjects.size());
  output->_colliders.resize(allHits.m_collisionObjects.size());
  output->_normals.resize(allHits.m_hitNormalWorld.size());
  for (u32 i = 0; i < allHits.m_collisionObjects.size(); ++i ) {
    const btVector3& n = allHits.m_hitNormalWorld[i];
    output->_rigidBodies[i] = static_cast<RigidBody*>(allHits.m_collisionObjects[i]->getUserPointer());
    output->_colliders[i] = output->_rigidBodies[i]->GetCollider();
    output->_normals[i] = Vector3(n.x(), n.y(), n.z());
  }
  return allHits.hasHit();
}


void BulletPhysics::ClearForces(RigidBody* body)
{
  R_ASSERT(body, "Rigid Body was null.");
  uuid64 k = body->GetUUID();
  btRigidBody* rb = kRigidBodyMap[k].native;
  rb->clearForces();
}


CompoundCollider* BulletPhysics::CreateCompoundCollider()
{
  CompoundCollider* collider = new CompoundCollider();
  btCompoundShape* pCompound = new btCompoundShape();
  kCollisionShapes[collider->GetUUID()] = pCompound;

  return collider;
}


SphereCollider* BulletPhysics::CreateSphereCollider(r32 radius)
{
  SphereCollider* sphere = new SphereCollider(radius);
  btSphereShape* nativeSphere = new btSphereShape(btScalar(radius));
  kCollisionShapes[sphere->GetUUID()] = nativeSphere;
  
  return sphere;
}


void BulletPhysics::AddCollider(RigidBody* body, Collider* collider)
{
  if (!collider || !body) return;

  physics_uuid_t uuid = body->GetUUID();
  RigidBundle& bundle = kRigidBodyMap[uuid];
  btCollisionShape* shape = kCollisionShapes[collider->GetUUID()];
  btTransform localTransform;
  localTransform.setIdentity();
  Vector3 center = collider->GetCenter();
  localTransform.setOrigin(btVector3(
    btScalar(center.x),
    btScalar(center.y),
    btScalar(center.z)
  ));
  bundle.compound->addChildShape(localTransform, shape);

  btVector3 inertia;
  r32 mass = bundle.rigidBody->m_mass;
  bundle.compound->calculateLocalInertia(btScalar(mass),inertia);

  bundle.native->setMassProps(btScalar(mass), inertia);
  bundle.native->updateInertiaTensor();
}


void BulletPhysics::FreeCollider(Collider* collider)
{
  if (!collider) return;
  auto it = kCollisionShapes.find(collider->GetUUID());
  if (it == kCollisionShapes.end()) return;

  btCollisionShape* native = it->second;
  kCollisionShapes.erase(it);

  delete native;
  delete collider;
}
} // Recluse