cmake_minimum_required(VERSION 3.0)
project("Physics")

set(PHYSICS_PUBLIC_DIR     Public/Physics)
set(PHYSICS_PRIVATE_DIR    Private)

if (${RECLUSE_BULLET_DIR} STREQUAL "")
  message(ERROR_FATAL "Bullet dir is not valid! Need to specify BULLETSDK in system enviroment variables!")
endif()

message("Searching Bullet directory: ${RECLUSE_BULLET_DIR}")

string(REPLACE "\\" "/" RECLUSE_PHYSX_DIR  ${RECLUSE_BULLET_DIR})
set(BULLET_LIB_DIR            ${RECLUSE_BULLET_DIR}/lib)

set(PHYSICS_FILES
  ${PHYSICS_PUBLIC_DIR}/Physics.hpp
  ${PHYSICS_PUBLIC_DIR}/BoxCollider.hpp
  ${PHYSICS_PUBLIC_DIR}/SphereCollider.hpp
  ${PHYSICS_PUBLIC_DIR}/Collider.hpp
  ${PHYSICS_PUBLIC_DIR}/Collision.hpp
  ${PHYSICS_PUBLIC_DIR}/PhysicsConfigs.hpp
  ${PHYSICS_PUBLIC_DIR}/RigidBody.hpp
  ${PHYSICS_PRIVATE_DIR}/BulletPhysics.hpp

  ${PHYSICS_PRIVATE_DIR}/BulletPhysics.cpp
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
  # Include Bullet directory.
  ${RECLUSE_BULLET_DIR}/src
)


set(BULLET_LIBS
  optimized ${RECLUSE_BULLET_DIR}/lib/Release/Bullet3Collision.lib
  optimized ${RECLUSE_BULLET_DIR}/lib/Release/Bullet3Common.lib
  optimized ${RECLUSE_BULLET_DIR}/lib/Release/Bullet3Dynamics.lib
  optimized ${RECLUSE_BULLET_DIR}/lib/Release/Bullet3Geometry.lib
  optimized ${RECLUSE_BULLET_DIR}/lib/Release/BulletCollision.lib
  optimized ${RECLUSE_BULLET_DIR}/lib/Release/BulletDynamics.lib
)

set(BULLET_LIBS_DEBUG
)

target_link_libraries(${RECLUSE_PHYSICS}
  ${BULLET_LIBS_DEBUG}
  ${BULLET_LIBS}
)

message("Bullet libraries found.")

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