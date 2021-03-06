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
  ${RECLUSE_GAME_PUBLIC_DIR}/Character.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/RendererComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/LightComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/PointLightComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/DirectionLightComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/CameraComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/AudioComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/AnimationComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/MeshComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/PhysicsComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/ParticleSystemComponent.hpp
  ${RECLUSE_GAME_PUBLIC_DIR}/AIComponent.hpp
  
  ${RECLUSE_GAME_PRIVATE_DIR}/GameObject.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/Camera.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/Engine.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/RendererComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/GameObjectManager.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/LightComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/CameraComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/MeshComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/Component.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/PointLightComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/PhysicsComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/AudioComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/AnimationComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/ParticleSystemComponent.cpp
  ${RECLUSE_GAME_PRIVATE_DIR}/AIComponent.cpp

  ${GEOMETRY_PUBLIC_DIR}/Cube.hpp
  ${GEOMETRY_PUBLIC_DIR}/UVSphere.hpp
  ${GEOMETRY_PUBLIC_DIR}/GeoSphere.hpp

  ${GEOMETRY_PRIVATE_DIR}/Cube.cpp
  ${GEOMETRY_PRIVATE_DIR}/UVSphere.cpp
  ${GEOMETRY_PRIVATE_DIR}/GeoSphere.cpp
  
  ${SCENE_PUBLIC_DIR}/Scene.hpp
  ${SCENE_PUBLIC_DIR}/SceneCache.hpp
  ${SCENE_PUBLIC_DIR}/ModelLoader.hpp
  ${SCENE_PUBLIC_DIR}/AssetManager.hpp
  ${SCENE_PRIVATE_DIR}/Scene.cpp
  ${SCENE_PRIVATE_DIR}/ModelLoader.cpp
  ${SCENE_PRIVATE_DIR}/ModelLoaderGLTF.cpp
  ${SCENE_PRIVATE_DIR}/ModelLoaderGLTF.hpp
  ${SCENE_PRIVATE_DIR}/tiny_gltf.hpp
  ${SCENE_PRIVATE_DIR}/tiny_gltf.cpp
  ${SCENE_PRIVATE_DIR}/json.hpp
  ${SCENE_PRIVATE_DIR}/stb_image_write.hpp
  ${SCENE_PRIVATE_DIR}/AssetManager.cpp
  
  ${SCRIPTS_PUBLIC_DIR}/Behavior.hpp
  ${SCRIPTS_PUBLIC_DIR}/ScriptCache.hpp
  
  ${VISIBLE_PUBLIC_DIR}/Octree.hpp

  ${VISIBLE_PRIVATE_DIR}/Octree.cpp

  ${RENDERING_PUBLIC_DIR}/TextureCache.hpp
  ${RENDERING_PUBLIC_DIR}/RendererResourcesCache.hpp
  ${RENDERING_PUBLIC_DIR}/Sky.hpp

  ${RENDERING_PRIVATE_DIR}/TextureCache.cpp
  ${RENDERING_PRIVATE_DIR}/RendererResourcesCache.cpp
  ${RENDERING_PRIVATE_DIR}/Sky.cpp

)

if (USE_FBX)
set(GAME_FILES ${GAME_FILES}
  ${SCENE_PRIVATE_DIR}/ModelLoaderFBX.cpp
  ${SCENE_PRIVATE_DIR}/ModelLoaderFBX.hpp
)
endif()


add_library(${RECLUSE_GAME} STATIC
  ${GAME_FILES}
)

if (USE_FBX)
target_link_libraries(${RECLUSE_GAME}
  optimized ${RECLUSE_FBX_DIR}/lib/vs2015/x64/release/libfbxsdk-mt.lib
  debug ${RECLUSE_FBX_DIR}/lib/vs2015/x64/debug/libfbxsdk-mt.lib
)
include_directories(${RECLUSE_FBX_DIR}/include)
endif()

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