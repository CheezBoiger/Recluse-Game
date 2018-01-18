cmake_minimum_required(VERSION 3.0)
project("GameRecluse")

set(RECLUSE_GAME_PUBLIC_DIR  Public/Game)
set(RECLUSE_GAME_PRIVATE_DIR Private)

set(SCENE_PUBLIC_DIR       ${RECLUSE_GAME_PUBLIC_DIR}/Scene)
set(ASSETS_PUBLIC_DIR      ${RECLUSE_GAME_PUBLIC_DIR}/Assets)
set(SCRIPTS_PUBLIC_DIR     ${RECLUSE_GAME_PUBLIC_DIR}/Scripts)
set(GEOMETRY_PUBLIC_DIR    ${RECLUSE_GAME_PUBLIC_DIR}/Geometry)
set(RENDERING_PUBLIC_DIR   ${RECLUSE_GAME_PUBLIC_DIR}/Rendering)
set(VISIBLE_PUBLIC_DIR     ${RECLUSE_GAME_PUBLIC_DIR}/Visibility)

set(VISIBLE_PRIVATE_DIR    ${RECLUSE_GAME_PRIVATE_DIR}/Visibility)
set(SCENE_PRIVATE_DIR      ${RECLUSE_GAME_PRIVATE_DIR}/Scene)
set(ASSETS_PRIVATE_DIR     ${RECLUSE_GAME_PRIVATE_DIR}/Assets)
set(RENDERING_PRIVATE_DIR  ${RECLUSE_GAME_PRIVATE_DIR}/Rendering)
set(SCRIPTS_PRIVATE_DIR    ${RECLUSE_GAME_PRIVATE_DIR}/Scripts)
set(GEOMETRY_PRIVATE_DIR   ${RECLUSE_GAME_PRIVATE_DIR}/Geometry)

include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${RECLUSE_GAME_PUBLIC_DIR}
)

set(GAME_FILES
  ${RECLUSE_GAME_PUBLIC_DIR}/Camera.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/Engine.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/GameObject.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/GameObjectManager.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/Component.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/CameraViewFrustum.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/Character.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/RendererComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/LightComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/PointLightComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/DirectionLightComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/CameraComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/AudioSourceComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/MeshComponent.hpp

  ${RECLUSE_GAME_PRIVATE_DIR}/GameObject.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/Camera.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/Engine.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/RendererComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/GameObjectManager.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/LightComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/CameraComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/CameraViewFrustum.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/MeshComponent.cpp

  ${GEOMETRY_PUBLIC_DIR}/Cube.hpp
  ${GEOMETRY_PUBLIC_DIR}/UVSphere.hpp
  ${GEOMETRY_PUBLIC_DIR}/GeoSphere.hpp

  ${GEOMETRY_PRIVATE_DIR}/Cube.cpp
  ${GEOMETRY_PRIVATE_DIR}/UVSphere.cpp
  ${GEOMETRY_PRIVATE_DIR}/GeoSphere.cpp
  
  ${SCENE_PUBLIC_DIR}/Scene.hpp
  ${SCENE_PUBLIC_DIR}/SceneCache.hpp

  ${SCENE_PRIVATE_DIR}/Scene.cpp
  
  ${SCRIPTS_PUBLIC_DIR}/Behavior.hpp
  ${SCRIPTS_PUBLIC_DIR}/ScriptCache.hpp

  ${VISIBLE_PUBLIC_DIR}/Octree.hpp

  ${VISIBLE_PRIVATE_DIR}/Octree.cpp

  ${RENDERING_PUBLIC_DIR}/TextureCache.hpp
  ${RENDERING_PRIVATE_DIR}/TextureCache.cpp
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