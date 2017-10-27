cmake_minimum_required(VERSION 3.0)
project("Physics")

set(PHYSICS_PUBLIC_DIR     Public/Physics)
set(PHYSICS_PRIVATE_DIR    Private)

if (${RECLUSE_PHYSX_DIR} STREQUAL "")
  message(ERROR_FATAL "PhysX dir is not valid! Need to specify PHYSXSDK in system enviroment variables!")
endif()

message("Searching PhysX directory: ${RECLUSE_PHYSX_DIR}")

string(REPLACE "\\" "/" RECLUSE_PHYSX_DIR  ${RECLUSE_PHYSX_DIR})
set(PHYSX_LIB_DIR            ${RECLUSE_PHYSX_DIR}/PhysX_3.4/Lib/vc14win64)
set(PHYSX_FOUNDATION_LIB_DIR ${RECLUSE_PHYSX_DIR}/PxShared/lib/vc14win64)

set(PHYSICS_FILES
  ${PHYSICS_PUBLIC_DIR}/Physics.hpp
  ${PHYSICS_PUBLIC_DIR}/BoxCollider.hpp
  ${PHYSICS_PUBLIC_DIR}/SphereCollider.hpp
  ${PHYSICS_PUBLIC_DIR}/Collider.hpp
  ${PHYSICS_PUBLIC_DIR}/Collision.hpp
  ${PHYSICS_PUBLIC_DIR}/PhysicsConfigs.hpp
  ${PHYSICS_PUBLIC_DIR}/RigidBody.hpp
  ${PHYSICS_PRIVATE_DIR}/PhysX.hpp

  ${PHYSICS_PRIVATE_DIR}/PhysX.cpp
  ${PHYSICS_PRIVATE_DIR}/Physics.cpp
  ${PHYSICS_PRIVATE_DIR}/BoxCollider.cpp
  ${PHYSICS_PRIVATE_DIR}/SphereCollider.cpp
  ${PHYSICS_PRIVATE_DIR}/Collision.cpp
  ${PHYSICS_PRIVATE_DIR}/RigidBody.cpp
  ${PHYSICS_PRIVATE_DIR}/PhysicsMesh.cpp
)


add_library(${RECLUSE_PHYSICS} STATIC
  ${PHYSICS_FILES}
)


target_include_directories(${RECLUSE_PHYSICS} PRIVATE
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${PHYSICS_PUBLIC_DIR}
  # Must add physX
  ${RECLUSE_PHYSX_DIR}/PhysX_3.4/Include
  ${RECLUSE_PHYSX_DIR}/PxShared/include
)


set(PHYSX_LIBS
  optimized ${PHYSX_LIB_DIR}/PhysX3Gpu_x64.lib
  optimized ${PHYSX_FOUNDATION_LIB_DIR}/PxCudaContextManager_x64.lib
  optimized ${PHYSX_FOUNDATION_LIB_DIR}/PxFoundation_x64.lib
  optimized ${PHYSX_LIB_DIR}/PhysX3_x64.lib
  optimized ${PHYSX_LIB_DIR}/PhysX3Common_x64.lib
  optimized ${PHYSX_LIB_DIR}/PhysX3Extensions.lib
  optimized ${PHYSX_LIB_DIR}/PhysX3CharacterKinematic_x64.lib
  optimized ${PHYSX_LIB_DIR}/LowLevelParticles.lib
  optimized ${PHYSX_LIB_DIR}/LowLevelCloth.lib
  optimized ${PHYSX_LIB_DIR}/LowLevelAABB.lib
  optimized ${PHYSX_LIB_DIR}/LowLevel.lib
  optimized ${PHYSX_LIB_DIR}/PhysX3Vehicle.lib
  optimized ${PHYSX_FOUNDATION_LIB_DIR}/PxTask_x64.lib
  optimized ${PHYSX_FOUNDATION_LIB_DIR}/PxPvdSDK_x64.lib
)

set(PHYSX_LIBS_DEBUG
  debug ${PHYSX_LIB_DIR}/PhysX3GpuDEBUG_x64.lib
  debug ${PHYSX_FOUNDATION_LIB_DIR}/PxCudaContextManagerDEBUG_x64.lib
  debug ${PHYSX_FOUNDATION_LIB_DIR}/PxFoundationDEBUG_x64.lib
  debug ${PHYSX_LIB_DIR}/PhysX3DEBUG_x64.lib
  debug ${PHYSX_LIB_DIR}/PhysX3CommonDEBUG_x64.lib
  debug ${PHYSX_LIB_DIR}/PhysX3ExtensionsDEBUG.lib
  debug ${PHYSX_LIB_DIR}/PhysX3CharacterKinematicDEBUG_x64.lib
  debug ${PHYSX_LIB_DIR}/LowLevelParticlesDEBUG.lib
  debug ${PHYSX_LIB_DIR}/LowLevelClothDEBUG.lib
  debug ${PHYSX_LIB_DIR}/LowLevelAABBDEBUG.lib
  debug ${PHYSX_LIB_DIR}/LowLevelDEBUG.lib
  debug ${PHYSX_LIB_DIR}/PhysX3VehicleDEBUG.lib
  debug ${PHYSX_FOUNDATION_LIB_DIR}/PxTaskDEBUG_x64.lib
  debug ${PHYSX_FOUNDATION_LIB_DIR}/PxPvdSDKDEBUG_x64.lib
)

target_link_libraries(${RECLUSE_PHYSICS}
  ${PHYSX_LIBS_DEBUG}
  ${PHYSX_LIBS}
)

message("PhysX libraries found.")

if (MSVC)
  foreach(source IN LISTS PHYSICS_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()

  set_property(TARGET ${RECLUSE_PHYSICS}
    PROPERTY FOLDER ${RECLUSE_ENGINE}
  )
endif()