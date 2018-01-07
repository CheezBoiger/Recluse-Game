cmake_minimum_required(VERSION 3.0)
project("Animation")


set(ANIMATION_PUBLIC_DIR   Public/Animation)
set(ANIMATION_PRIVATE_DIR  Private)

include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${ANIMATION_PUBLIC_DIR}
)


set(ANIMATION_FILES
  ${ANIMATION_PUBLIC_DIR}/Animation.hpp
  ${ANIMATION_PUBLIC_DIR}/Clip.hpp
  ${ANIMATION_PUBLIC_DIR}/Skeleton.hpp

  ${ANIMATION_PRIVATE_DIR}/Animation.cpp
)


add_library(${RECLUSE_ANIMATION} STATIC
  ${ANIMATION_FILES}
)



# Doing some stuff for organization.
if (MSVC)
  foreach(source IN LISTS ANIMATION_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()
endif()

# Visual Studio organization. For Good purposes.
if (WIN32)
  set_property(TARGET ${RECLUSE_RENDERER} 
    PROPERTY 
    FOLDER ${RECLUSE_NAME})
endif()