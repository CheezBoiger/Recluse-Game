cmake_minimum_required(VERSION 3.0)
project("Audio")


set(AUDIO_PUBLIC_DIR         Public/Audio)
set(AUDIO_PRIVATE_DIR        Private)

string(REPLACE "\\" "/" RECLUSE_OPENAL_DIR ${RECLUSE_OPENAL_DIR})
set(OPENAL_HEADER_DIR ${RECLUSE_OPENAL_DIR}/include)
set(OPENAL_LIB_DIR ${RECLUSE_OPENAL_DIR}/libs/Win64)

set(FMOD_LOWLEVEL_HEADER_DIR ${RECLUSE_FMOD_DIR}/api/lowlevel/inc)
set(FMOD_LOWLEVEL_LIB_DIR    ${RECLUSE_FMOD_DIR}/api/lowlevel/lib)

set(FMOD_STUDIO_HEADER_DIR ${RECLUSE_FMOD_DIR}/api/studio/inc)
set(FMOD_STUDIO_LIB_DIR    ${RECLUSE_FMOD_DIR}/api/studio/lib)


include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${AUDIO_PUBLIC_DIR}
  ${FMOD_LOWLEVEL_HEADER_DIR}
  ${FMOD_STUDIO_HEADER_DIR}
)


set(OPENAL_AUDIO_LIBS
  ${OPENAL_LIB_DIR}/OpenAL32.lib
  ${OPENAL_LIB_DIR}/EFX-Util_MT/EFX-Util.lib
)

set(FMOD_AUDIO_LIBS
  ${FMOD_LOWLEVEL_LIB_DIR}/fmod64_vc.lib
  ${FMOD_LOWLEVEL_LIB_DIR}/fmodL64_vc.lib
  
  ${FMOD_STUDIO_LIB_DIR}/fmodstudio64_vc.lib
  ${FMOD_STUDIO_LIB_DIR}/fmodstudioL64_vc.lib
)


set(AUDIO_FILES
  ${AUDIO_PUBLIC_DIR}/Audio.hpp
  ${AUDIO_PUBLIC_DIR}/AudioListener.hpp
  ${AUDIO_PUBLIC_DIR}/AudioConfigs.hpp
  ${AUDIO_PUBLIC_DIR}/AudioSource.hpp
  
  ${AUDIO_PRIVATE_DIR}/Audio.cpp
  ${AUDIO_PRIVATE_DIR}/AudioListener.cpp
  ${AUDIO_PRIVATE_DIR}/AudioSource.cpp
  ${AUDIO_PRIVATE_DIR}/AudioEngine.hpp
  ${AUDIO_PRIVATE_DIR}/AudioEngine.cpp
)


add_library(${RECLUSE_AUDIO} STATIC
  ${AUDIO_FILES}
)


target_link_libraries(${RECLUSE_AUDIO}
  ${FMOD_AUDIO_LIBS}
)

# Doing some stuff for organization.
if (MSVC)
  foreach(source IN LISTS AUDIO_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()
endif()

# Visual Studio organization. For Good purposes.
if (WIN32)
  set_property(TARGET ${RECLUSE_AUDIO} 
    PROPERTY 
    FOLDER ${RECLUSE_NAME})
endif()

