cmake_minimum_required(VERSION 3.0)
project("UI")


set(UI_PUBLIC_DIR   Public/UI)
set(UI_PRIVATE_DIR  Private)


set(UI_FILES
  ${UI_PUBLIC_DIR}/FontManager.hpp
  ${UI_PUBLIC_DIR}/UI.hpp
  ${UI_PUBLIC_DIR}/Canvas.hpp
  ${UI_PUBLIC_DIR}/GUIImage.hpp
  ${UI_PUBLIC_DIR}/Text.hpp
  ${UI_PUBLIC_DIR}/UIButton.hpp

  ${UI_PRIVATE_DIR}/UI.cpp
  ${UI_PRIVATE_DIR}/FreeType.hpp
  ${UI_PRIVATE_DIR}/FreeType.cpp
  ${UI_PRIVATE_DIR}/FontManager.cpp
)


add_library(${RECLUSE_UI} STATIC
  ${UI_FILES}
)


target_include_directories(${RECLUSE_UI} PRIVATE
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${UI_PUBLIC_DIR}
  ${RECLUSE_FREETYPE_DIR}/include
)


target_link_libraries(${RECLUSE_UI}
  ${RECLUSE_FREETYPE_DIR}/x64/freetype28MT.lib
)


# Doing some stuff for organization.
if (MSVC)
  foreach(source IN LISTS UI_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()
endif()

# Visual Studio organization. For Good purposes.
if (WIN32)
  set_property(TARGET ${RECLUSE_UI} 
    PROPERTY 
    FOLDER ${RECLUSE_NAME})
endif()