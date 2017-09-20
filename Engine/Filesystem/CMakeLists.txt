cmake_minimum_required(VERSION 3.0)
project("Filesystem")

set(FILESYSTEM_PUBLIC_DIR  Public/Filesystem)
set(FILESYSTEM_PRIVATE_DIR Private)

include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${FILESYSTEM_PUBLIC_DIR}
)


set(FILESYSTEM_FILES
  ${FILESYSTEM_PUBLIC_DIR}/File.hpp
  ${FILESYSTEM_PUBLIC_DIR}/FileCache.hpp
  ${FILESYSTEM_PUBLIC_DIR}/Filesystem.hpp

  ${FILESYSTEM_PRIVATE_DIR}/File.cpp
  ${FILESYSTEM_PRIVATE_DIR}/FileCache.cpp
  ${FILESYSTEM_PRIVATE_DIR}/Filesystem.cpp
)


add_library(${RECLUSE_FILESYSTEM} STATIC
  ${FILESYSTEM_FILES}
)

if (MSVC)
  foreach(source IN LISTS FILESYSTEM_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()

  set_property(TARGET ${RECLUSE_FILESYSTEM}
    PROPERTY FOLDER ${RECLUSE_ENGINE}
  )
endif()


