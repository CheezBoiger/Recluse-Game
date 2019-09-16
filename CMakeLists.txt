cmake_minimum_required(VERSION 3.0)
project("Recluse")

# optional params during project build.
option(PHYSICS             "Turns physics compilation and use ON/OFF" ON)
option(AUDIO               "Turns audio compilation and use ON/OFF" ON)

set(RECLUSE_NAME            "Recluse")
set(RECLUSE_EXE             ${RECLUSE_NAME})
set(RECLUSE_ENGINE          "Engine")

set(RECLUSE_LIB_DIR         ${CMAKE_SOURCE_DIR}/Libs)
set(RECLUSE_SOURCE_DIR      ${CMAKE_SOURCE_DIR}/Engine)
set(RECLUSE_TEST_DIR        ${CMAKE_SOURCE_DIR}/Test)
set(RECLUSE_REGRESSION_DIR  ${CMAKE_SOURCE_DIR}/Regression)
set(RECLUSE_GAME_DIR        ${CMAKE_SOURCE_DIR}/Game)

set(RECLUSE_TINYOBJ__DIR    ${RECLUSE_LIB_DIR}/TinyObjLoader)
set(RECLUSE_OZZ_DIR         ${RECLUSE_LIB_DIR}/Ozz)
set(RECLUSE_FREETYPE_DIR    ${RECLUSE_LIB_DIR}/FreeType)

if (WIN32)
 set(RECLUSE_WIN32_RC_FILE ${CMAKE_SOURCE_DIR}/Assets/App/Win32/win32.rc)
else()
 set(RECLUSE_WIN32_RC_FILE "")
endif()

# Libraries that REQUIRE enviroment variables set up.
if (PHYSICS)
  set(RECLUSE_BULLET_DIR       $ENV{BULLETSDK})
  set(BULLET_INCLUDE           ${RECLUSE_BULLET_DIR}/include)
else()
  set(RECLUSE_BULLET_DIR       "")
  set(BULLET_INCLUDE           "")
  add_definitions(-DFORCE_PHYSICS_OFF=1)
endif()

if (USE_FBX)
  set(RECLUSE_FBX_DIR                 $ENV{FBXSDK})
  set(FBX_INCLUDE                     )
  add_definitions(-DINCLUDE_FBX=1)
else()
  set(RECLUSE_FBX_DIR                 "")
  set(FBX_INCLUDE                     "")
  add_definitions(-DINCLUDE_FBX=0)
endif(USE_FBX)

if (AUDIO)
  set(RECLUSE_OPENAL_DIR       $ENV{OPENALSDK})
  set(RECLUSE_FMOD_DIR         $ENV{FMODSDK})
  set(RECLUSE_USE_FMOD         ON)
  add_definitions(-DRECLUSE_TYPE_FMOD=1
                  -DRECLUSE_TYPE_AL=2
    -DRECLUSE_AUDIO_TYPE=RECLUSE_TYPE_FMOD)
else()
  set(RECLUSE_OPENAL_DIR       "")
  set(RECLUSE_FMOD_DIR         "")
  set(RECUSE_USE_FMOD          OFF)
  add_definitions(-DFORCE_AUDIO_OFF=1)
endif()

set(RECLUSE_THIRDPARTY_INCLUDE_DIRS
  ${RECLUSE_FREETYPE_DIR}/include
  ${BULLET_INCLUDE}
)

set(RECLUSE_RENDERER     "Renderer")
set(RECLUSE_PHYSICS      "Physics")
set(RECLUSE_GAME         "Recluse")
set(RECLUSE_SHADER       "Shaders")
set(RECLUSE_CORE         "Core")
set(RECLUSE_AUDIO        "Audio")
set(RECLUSE_ANIMATION    "Animation")
set(RECLUSE_FILESYSTEM   "Filesystem")
set(RECLUSE_UI           "UI")
set(RECLUSE_SHADERS      "Shaders")
set(RECLUSE_AI           "AI")

set(RECLUSE_RENDERER_INCLUDE_DIR   ${RECLUSE_SOURCE_DIR}/Renderer/Public)
set(RECLUSE_PHYSICS_INCLUDE_DIR    ${RECLUSE_SOURCE_DIR}/Physics/Public)
set(RECLUSE_ANIMATION_INCLUDE_DIR  ${RECLUSE_SOURCE_DIR}/Animation/Public)
set(RECLUSE_AUDIO_INCLUDE_DIR      ${RECLUSE_SOURCE_DIR}/Audio/Public)
set(RECLUSE_CORE_INCLUDE_DIR       ${RECLUSE_SOURCE_DIR}/Core/Public)
set(RECLUSE_SHADER_DIR             ${RECLUSE_SOURCE_DIR}/Shader)
set(RECLUSE_GAME_INCLUDE_DIR       ${RECLUSE_SOURCE_DIR}/Game/Public)
set(RECLUSE_UI_INCLUDE_DIR         ${RECLUSE_SOURCE_DIR}/UI/Public)
set(RECLUSE_AI_INCLUDE_DIR         ${RECLUSE_SOURCE_DIR}/AI/Public)
set(RECLUSE_FILESYSTEM_INCLUDE_DIR ${RECLUSE_SOURCE_DIR}/Filesystem/Public)
set(RECLUSE_SHADERS_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders)

set(RECLUSE_ENGINE_INCLUDE_DIRS
  ${RECLUSE_THIRDPARTY_INCLUDE_DIRS}
  ${RECLUSE_RENDERER_INCLUDE_DIR}
  ${RECLUSE_PHYSICS_INCLUDE_DIR}
  ${RECLUSE_ANIMATION_INCLUDE_DIR}
  ${RECLUSE_AUDIO_INCLUDE_DIR}
  ${RECLUSE_CORE_INCLUDE_DIR}
  ${RECLUSE_SHADER_DIR}
  ${RECLUSE_GAME_INCLUDE_DIR}
  ${RECLUSE_UI_INCLUDE_DIR}
  ${RECLUSE_AI_INCLUDE_DIR}
  ${RECLUSE_FILESYSTEM_INCLUDE_DIR}
)

set(RECLUSE_ENGINE_LINK_LIBRARIES
  ${RECLUSE_RENDERER}
  ${RECLUSE_GAME}
  ${RECLUSE_ANIMATION}
  ${RECLUSE_CORE}
  ${RECLUSE_UI}
  ${RECLUSE_FILESYSTEM}
  ${RECLUSE_AI}
)

if (PHYSICS)
  set(RECLUSE_ENGINE_LINK_LIBRARIES 
    ${RECLUSE_ENGINE_LINK_LIBRARIES}
    ${RECLUSE_PHYSICS}
  )
endif()

if (AUDIO)
  set(RECLUSE_ENGINE_LINK_LIBRARIES 
    ${RECLUSE_ENGINE_LINK_LIBRARIES}
    ${RECLUSE_AUDIO}
  )
endif()

include(CMake/Macros.cmake)

# find vulkan render API.
find_package(Vulkan)

# Add directory.
add_subdirectory(${RECLUSE_SOURCE_DIR})
add_subdirectory(${RECLUSE_TEST_DIR})
add_subdirectory(${RECLUSE_REGRESSION_DIR})
add_subdirectory(${RECLUSE_GAME_DIR})