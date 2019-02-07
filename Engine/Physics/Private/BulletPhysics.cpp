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

std::vector<btRigidBody*>               kRigidBodies;
std::vector<RigidBody*>                 kEngineRigidBodies;
std::vector<Collider*>                  kEngineColliders;
//std::vector<btCollisionShape*>          kCollisionShapes;

// Global physics manager that holds physics contraint solvers, dispatchers, configuration
// and management.
struct bt_physics_manager 
{
  // 
  btDefaultCollisionConfiguration*            _pCollisionConfiguration;

  // 
  btCollisionDispatcher*                      _pDispatcher;

  //
  btBroadphaseInterface*                      _pOverlappingPairCache;

  // Interchangeable constraint solver. We will use Sequential Impulse, as it is
  // popular. We can use Projected Gauss-Seidel for experimentation later... 
  btSequentialImpulseConstraintSolver*        _pSolver;

  //
  btDiscreteDynamicsWorld*                    _pWorld;

  // World soft.
  btSoftRigidDynamicsWorld*                   _pSoftWorld;

  //
  btSoftBodyRigidBodyCollisionConfiguration*  _pSoftCollisionConfiguration;
} bt_manager;


RigidBody* GetRigidBody(physics_uuid_t key)
{
  RigidBody* body = nullptr;

  if (kRigidBodyMap.find(key) != kRigidBodyMap.end()) {
    body = kRigidBodyMap[key].rigidBody;  
  }

  return body;
}


RigidBundle* GetRigidBundle(physics_uuid_t key)
{
  RigidBundle* pBundle = nullptr;
  auto it = kRigidBodyMap.find(key);
  if (it != kRigidBodyMap.end()) pBundle = &it->second;
  return pBundle;
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
  } else {
    Physics::OnStartUp();
  }
  Initialize();

  R_DEBUG(rNotify, "Physics Engine is successfully initialized.\n");
}


void BulletPhysics::OnShutDown()
{
  Physics::OnShutDown();
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
    btScalar(rigidbody->_mass),
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
  collider->SetExtent(scale);
  return collider;
}


void BulletPhysics::FreeRigidBody(RigidBody* body)
{
  if (!body) return;
  R_DEBUG(rVerbose, "Freeing rigid body.\n");
  physics_uuid_t uuid = body->GetUUID();
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

    collisionOnA._gameObject = bodyB->_gameObj;
    collisionOnA._rigidBody = bodyB;

    collisionOnB._gameObject = bodyA->_gameObj;
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
    btVector3 velocity = bundle.native->getLinearVelocity();
    btVector3 angularVelocity = bundle.native->getAngularVelocity();

    bundle.rigidBody->_rotation = Quaternion(q.x(), q.y(), q.z(), q.w());
    bundle.rigidBody->_position = Vector3(p.x(), p.y(), p.z());
    bundle.rigidBody->_velocity = Vector3(velocity.x(), velocity.y(), velocity.z());    
  }
}


void BulletPhysics::SetMass(RigidBody* body, r32 mass)
{
  if (!body) return;
  physics_uuid_t key = body->GetUUID();
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
  physics_uuid_t key = body->GetUUID();
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
  if (body->_activated) obj->activate();
  
}


void BulletPhysics::ActivateRigidBody(RigidBody* body)
{
  if (!body) return;
  physics_uuid_t key = body->GetUUID();
  btRigidBody* rb = kRigidBodyMap[key].native;
  body->_activated = true;
  rb->activate();
}


void BulletPhysics::DeactivateRigidBody(RigidBody* body)
{
  if (!body) return;
  physics_uuid_t key = body->GetUUID();
  btRigidBody* rb = kRigidBodyMap[key].native;
  body->_activated = false;
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
  physics_uuid_t k = body->GetUUID();
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
  output->_collider = &rbHit->_compound;
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
  for (i32 i = 0; i < allHits.m_collisionObjects.size(); ++i ) {
    const btVector3& n = allHits.m_hitNormalWorld[i];
    output->_rigidBodies[i] = static_cast<RigidBody*>(allHits.m_collisionObjects[i]->getUserPointer());
    output->_colliders[i] = &output->_rigidBodies[i]->_compound;
    output->_normals[i] = Vector3(n.x(), n.y(), n.z());
  }
  return allHits.hasHit();
}


void BulletPhysics::ClearForces(RigidBody* body)
{
  R_ASSERT(body, "Rigid Body was null.");
  physics_uuid_t k = body->GetUUID();
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
  sphere->SetRadius(radius);
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
  r32 mass = bundle.rigidBody->_mass;
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


void BulletPhysics::SetFriction(RigidBody* body, r32 friction)
{
  R_ASSERT(body, "Body is null.");
  physics_uuid_t uuid = body->GetUUID();
  RigidBundle* bundle = GetRigidBundle(uuid);
  btRigidBody* native = bundle->native;
  native->setFriction(btScalar(friction));
}


void BulletPhysics::SetRollingFriction(RigidBody* body, r32 friction)
{
  R_ASSERT(body, "Body is null.");
  physics_uuid_t uuid = body->GetUUID();
  RigidBundle* bundle = GetRigidBundle(uuid);
  btRigidBody* native = bundle->native;
  native->setRollingFriction(friction);
}


void BulletPhysics::SetSpinningFriction(RigidBody* body, r32 friction)
{
  R_ASSERT(body, "Body is null.");
  physics_uuid_t uuid = body->GetUUID();
  RigidBundle* bundle = GetRigidBundle(uuid);
  btRigidBody* native = bundle->native;
  native->setSpinningFriction(friction);
}


void BulletPhysics::UpdateCompoundCollider(RigidBody* body, CompoundCollider* collider)
{
  R_ASSERT(body, "body is null.");
  btCompoundShape* pCompound = nullptr;

  {
    btCollisionShape* pShape = GetCollisionShape(collider);
    if (!pShape->isCompound()) {
      R_ASSERT(false, "Collider is not a compound shape!");
      return;
    }
    pCompound = static_cast<btCompoundShape*>(pShape);
  }

  i32 childCount = pCompound->getNumChildShapes();
  for (i32 i = 0; i < childCount; ++i) {
    btCollisionShape* pChild = pCompound->getChildShape(0);
    pCompound->removeChildShape(pChild);
  }

  auto& colliders = collider->GetColliders();
  for (auto pCollider : colliders) {
    btCollisionShape* shape = GetCollisionShape(pCollider);
    if (!shape) continue;
    btTransform localTransform;
    localTransform.setIdentity();
    Vector3 center = pCollider->GetCenter();
    localTransform.setOrigin(btVector3(
      btScalar(center.x),
      btScalar(center.y),
      btScalar(center.z)
    ));
    pCompound->addChildShape(localTransform, shape);
  }
}


void BulletPhysics::Reset(RigidBody* body)
{
  RigidBundle* bundle = GetRigidBundle(body->GetUUID());
  
  bt_manager._pWorld->removeRigidBody(bundle->native);
  btVector3 zeroV = btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f));
  bundle->native->clearForces();
  bundle->native->setLinearVelocity(zeroV);
  bundle->native->setAngularVelocity(zeroV);
  bt_manager._pWorld->addRigidBody(bundle->native);
}


void BulletPhysics::UpdateRigidBody(RigidBody* body, physics_update_bits_t bits)
{
  if (bits == 0) { return; }
  R_ASSERT(body, "Null rigid body sent to physics update.");
  R_DEBUG(rNotify, "Bullet physics rigid body update called by id: " + std::to_string(body->GetUUID()) + "\n");
  RigidBundle* bundle = GetRigidBundle(body->GetUUID());
  if (!bundle) return;
  btRigidBody* rigidBody = bundle->native;
  bt_manager._pWorld->removeRigidBody(rigidBody);

  if (bits & PHYSICS_UPDATE_RESET) {

    btVector3 zeroV = btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f));
    rigidBody->clearForces();
    rigidBody->setLinearVelocity(zeroV);
    rigidBody->setAngularVelocity(zeroV);
  }

  if (bits & PHYSICS_UPDATE_CLEAR_FORCES) {
    rigidBody->clearForces();
  }

  if (bits & PHYSICS_UPDATE_MASS) {
    btVector3 inertia;
    btCollisionShape* shape = rigidBody->getCollisionShape();
    shape->calculateLocalInertia(btScalar(body->_mass), inertia);
    rigidBody->setMassProps(btScalar(body->_mass), inertia);
  }

  if (bits & PHYSICS_UPDATE_FRICTION) {
    rigidBody->setFriction(btScalar(body->_friction));
  }

  if (bits & PHYSICS_UPDATE_ROLLING_FRICTION) { 
    rigidBody->setRollingFriction(btScalar(body->_rollingFriction));
  }

  if (bits & PHYSICS_UPDATE_SPINNING_FRICTION) {
    rigidBody->setSpinningFriction(btScalar(body->_spinningFriction));
  }

  if (bits & PHYSICS_UPDATE_LINEAR_VELOCITY) {
    
  }

  if (bits & PHYSICS_UPDATE_ANGULAR_VELOCITY) {
    
  }

  if (bits & PHYSICS_UPDATE_ANGLE_FACTOR) {
    rigidBody->setAngularFactor(
      btVector3(btScalar(body->_angleFactor.x),
        btScalar(body->_angleFactor.y),
        btScalar(body->_angleFactor.z)));
  }

  if (bits & PHYSICS_UPDATE_LINEAR_FACTOR) {
    rigidBody->setLinearFactor(
      btVector3(btScalar(body->_linearFactor.x),
        btScalar(body->_linearFactor.y),
        btScalar(body->_linearFactor.z)));
  }

  if (bits & PHYSICS_UPDATE_FORCES) {
    for (size_t i = 0; i < body->_forces.size(); ++i) {
      Vector3 force = body->_forces[i];
      Vector3 relPos = body->_forceRelativePositions[i];
      rigidBody->applyForce(btVector3(btScalar(force.x), btScalar(force.y), btScalar(force.z)),
        btVector3(btScalar(relPos.x), btScalar(relPos.y), btScalar(relPos.z)));
    }
    body->_forces.clear();
    body->_forceRelativePositions.clear();
  }

  if (bits & PHYSICS_UPDATE_IMPULSE) {
    for (size_t i = 0; i < body->_impulses.size(); ++i) {
      Vector3 impulse = body->_impulses[i];
      Vector3 relPos = body->_impulseRelativePositions[i];
      rigidBody->applyImpulse(btVector3(btScalar(impulse.x), btScalar(impulse.y), btScalar(impulse.z)),
        btVector3(btScalar(relPos.x), btScalar(relPos.y), btScalar(relPos.z)));
    }
    body->_impulses.clear();
    body->_impulseRelativePositions.clear();
  }

  bt_manager._pWorld->addRigidBody(rigidBody);
}
} // Recluse