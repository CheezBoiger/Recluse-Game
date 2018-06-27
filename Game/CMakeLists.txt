# Copyright (c) 2018 Recluse Project. All rights reserved.
cmake_minimum_required(VERSION 3.0)
project("Recluse-Game")



set(RS_SOURCE_DIR Source)
set(RS_CAMERA_DIR ${RS_SOURCE_DIR}/Camera)
set(RS_WEAPON_DIR ${RS_SOURCE_DIR}/Weapon)
set(RS_PLAYABLE_DIR ${RS_SOURCE_DIR}/Playable)


include_directories(${RECLUSE_ENGINE_INCLUDE_DIRS})

# Force static runtime libraries
foreach(flag
CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG_INIT
CMAKE_CXX_FLAGS_RELEASE  CMAKE_CXX_FLAGS_RELWITHDEBINFO
CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
  string(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
  set("${flag}" "${${flag}} /EHsc")
endforeach()


set(RS_EXE          "Recluse-Game")
set(RS_TARGET_FIILES
  ${RS_CAMERA_DIR}/PlayerCamera.hpp
  ${RS_CAMERA_DIR}/PlayerCamera.cpp
  
  ${RS_WEAPON_DIR}/IWeapon.hpp
  
  ${RS_SOURCE_DIR}/Main.cpp
)


# include here.

add_executable(${RS_EXE}
  ${RS_TARGET_FIILES}
)

target_link_libraries(${RS_EXE}
  ${RECLUSE_ENGINE_LINK_LIBRARIES}
)


copy_engine_dependencies_to_exe(${RS_EXE})