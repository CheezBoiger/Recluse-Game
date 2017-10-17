cmake_minimum_required(VERSION 3.0)
project("GameRecluse")

set(RECLUSE_GAME_PUBLIC_DIR  Public/Game)
set(RECLUSE_GAME_PRIVATE_DIR Private)

set(SCENE_PUBLIC_DIR     ${RECLUSE_GAME_PUBLIC_DIR}/Scene)
set(ASSETS_PUBLIC_DIR    ${RECLUSE_GAME_PUBLIC_DIR}/Assets)
set(SCRIPTS_PUBLIC_DIR   ${RELCUSE_GAME_PUBLIC_DIR}/Scripts)
set(GEOMETRY_PUBLIC_DIR  ${RECLUSE_GAME_PUBLIC_DIR}/Geometry)

set(SCENE_PRIVATE_DIR    ${RECLUSE_GAME_PRIVATE_DIR}/Scene)
set(ASSETS_PRIVATE_DIR   ${RECLUSE_GAME_PRIVATE_DIR}/Assets)
set(SCRIPTS_PRIVATE_DIR  ${RECLUSE_GAME_PRIVATE_DIR}/Scripts)
set(GEOMETRY_PRIVATE_DIR ${RECLUSE_GAME_PRIVATE_DIR}/Geometry)

include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${RECLUSE_GAME_PUBLIC_DIR}
)

set(GAME_FILES
  ${RECLUSE_GAME_PUBLIC_DIR}/Camera.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/Engine.hpp

  ${RECLUSE_GAME_PRIVATE_DIR}/Camera.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/Engine.cpp

  ${GEOMETRY_PUBLIC_DIR}/Cube.hpp

  ${GEOMETRY_PRIVATE_DIR}/Cube.cpp
)


add_library(${RECLUSE_GAME} STATIC
  ${GAME_FILES}
)


# Doing some stuff for organization.
if (MSVC)
  foreach(source IN LISTS GAME_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()
endif()

# Visual Studio organization. For Good purposes.
if (WIN32)
  set_property(TARGET ${RECLUSE_GAME} 
    PROPERTY 
    FOLDER ${RECLUSE_NAME})
endif()