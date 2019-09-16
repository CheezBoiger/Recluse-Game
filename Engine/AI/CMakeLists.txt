cmake_minimum_required(VERSION 3.0)
project("AI")

set(AI_PUBLIC_DIR Public/AI)
set(AI_PRIVATE_DIR Private)

include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${AI_PUBLIC_DIR}
)


set(AI_FILES
  ${AI_PUBLIC_DIR}/NavMesh.hpp
  ${AI_PUBLIC_DIR}/NavNode.hpp
  ${AI_PUBLIC_DIR}/PathFinding.hpp
  ${AI_PUBLIC_DIR}/AIEngine.hpp
  
  ${AI_PRIVATE_DIR}/NavMesh.cpp
  ${AI_PRIVATE_DIR}/PathFinding.cpp
  ${AI_PRIVATE_DIR}/AIEngine.cpp
)


add_library(${RECLUSE_AI} STATIC
  ${AI_FILES}
)

# Doing some stuff for organization.
if (MSVC)
  foreach(source IN LISTS AI_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()
endif()

# Visual Studio organization. For Good purposes.
if (WIN32)
  set_property(TARGET ${RECLUSE_AI} 
    PROPERTY 
    FOLDER ${RECLUSE_NAME})
endif()