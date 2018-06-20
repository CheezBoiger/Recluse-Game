# Copyright (c) 2018 Recluse Project. All rights reserved.
cmake_minimum_required(VERSION 3.0)
project("Recluse")



set(RS_SOURCE_DIR Source)
set(RS_CAMERA_DIR ${RS_SOURCE_DIR}/Camera)
set(RS_WEAPON_DIR ${RS_SOURCE_DIR}/Weapon)
set(RS_PLAYABLE_DIR ${RS_SOURCE_DIR}/Playable)


set(RS_EXE          "Recluse")
set(RS_TARGET_FIILES)


# include here.

add_executable(RS_EXE
  ${RS_TARGET_FIILES}
)


copy_engine_dependencies_to_exe(${RS_EXE})