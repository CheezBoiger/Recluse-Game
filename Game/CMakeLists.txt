# Copyright (c) 2018 Recluse Project. All rights reserved.
cmake_minimum_required(VERSION 3.0)
project("Recluse-Game")



set(RS_SOURCE_DIR Source)
set(RS_CAMERA_DIR ${RS_SOURCE_DIR}/Camera)
set(RS_WEAPON_DIR ${RS_SOURCE_DIR}/Weapon)
set(RS_PLAYABLE_DIR ${RS_SOURCE_DIR}/Playable)
set(RS_SCENES_DIR  ${RS_SOURCE_DIR}/Scenes)
set(RS_UI_DIR       ${RS_SOURCE_DIR}/UI)


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
  
  
  ${RS_PLAYABLE_DIR}/IActor.hpp
  ${RS_PLAYABLE_DIR}/Player.hpp
  ${RS_PLAYABLE_DIR}/Enemy.hpp
  ${RS_PLAYABLE_DIR}/Player.cpp
  
  ${RS_SCENES_DIR}/TestScene.hpp
  ${RS_SCENES_DIR}/TestScene.cpp
  
  ${RS_UI_DIR}/MenuHandle.hpp
  ${RS_UI_DIR}/MenuHandle.cpp
  ${RS_UI_DIR}/HUDDisplay.hpp
  ${RS_UI_DIR}/HUDDisplay.cpp
  
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