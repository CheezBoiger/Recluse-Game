// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "PhysX.hpp"
#include "Core/Exception.hpp"

#include "cudamanager/PxCudaContextManager.h"
#include "cudamanager/PxCudaMemoryManager.h"


namespace Recluse {


b8 PhysX::Initialize()
{
  m_pFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, m_DefaultAllocatorCallback, m_DefaultErrorCallback);
  m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, m_Tolerance);

  if (!m_pPhysics || !m_pFoundation) return false;
  return true;

  
  PxRegisterArticulations(*m_pPhysics);
  PxRegisterHeightFields(*m_pPhysics);
}


void PhysX::CleanUp()
{
  m_pPhysics->release();
  m_pFoundation->release();
}


PxScene* PhysX::CreateScene(PxVec3 gravity, PxSimulationFilterShader& filter,
  PxSceneFlags flags, u32 threads, b8 useGpu)
{
  // For now, since we don't want to tie with CUDA.
  useGpu = false;

  PxSceneDesc sceneDesc(m_pPhysics->getTolerancesScale());
  sceneDesc.gravity = gravity;
  sceneDesc.filterShader = filter;
  sceneDesc.flags |= flags;
  // Use GPU instead.
  if (useGpu) {
    PxCudaContextManager* cudaManager = nullptr;
    // TODO(): Figure out if we want to use cuda for our physics, for now let's
    // focus on the cpu side...
  } else {
    PxDefaultCpuDispatcher* cpuDispatcher = PxDefaultCpuDispatcherCreate(threads);
    if (!cpuDispatcher) {
      R_DEBUG(rError, "Failed to create a default cpu dispatcher for physics scene!\n");
    }
    sceneDesc.cpuDispatcher = cpuDispatcher;
  }

  PxScene* scene = m_pPhysics->createScene(sceneDesc);
  if (!scene) {
    R_DEBUG(rError, "Failed to create a physics scene!\n");
    return nullptr;
  }

  scene->setVisualizationParameter(PxVisualizationParameter::eSCALE,            1.0f);
  scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
  return scene;
}


void PhysX::Update(r32 stepTime)
{
  if (m_pPhysicsScene) {
    m_pPhysicsScene->simulate(stepTime);
    m_pPhysicsScene->fetchResults(true);
  }
}
} // Recluse