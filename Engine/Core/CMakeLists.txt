cmake_minimum_required(VERSION 3.0)
project("Core")

set(RECLUSE_CORE_PRIVATE_DIR  Private)
set(RECLUSE_CORE_PUB_DIR      Public/Core)

include_directories(
  ${RECLUSE_CORE_INCLUDE_DIR}/Core
  ${RECLUSE_CORE_INCLUDE_DIR}
)


set(RECLUSE_CORE_FILES
  ${RECLUSE_CORE_PUB_DIR}/Math/Matrix3.hpp
  ${RECLUSE_CORE_PUB_DIR}/Math/Matrix4.hpp
  ${RECLUSE_CORE_PUB_DIR}/Math/Quaternion.hpp
  ${RECLUSE_CORE_PUB_DIR}/Math/Ray.hpp
  ${RECLUSE_CORE_PUB_DIR}/Math/Vector2.hpp
  ${RECLUSE_CORE_PUB_DIR}/Math/Vector3.hpp
  ${RECLUSE_CORE_PUB_DIR}/Math/Vector4.hpp
  ${RECLUSE_CORE_PUB_DIR}/Math/Common.hpp

  ${RECLUSE_CORE_PRIVATE_DIR}/Math/Matrix3.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Math/Matrix4.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Math/Vector4.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Math/Vector3.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Math/Vector2.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Math/Quaternion.cpp
  #${RECLUSE_CORE_PRIVATE_DIR}/Math/Ray.cpp

  ${RECLUSE_CORE_PUB_DIR}/Utility/Vector.hpp
  ${RECLUSE_CORE_PUB_DIR}/Utility/Module.hpp
  ${RECLUSE_CORE_PUB_DIR}/Utility/Time.hpp
  ${RECLUSE_CORE_PUB_DIR}/Utility/Image.hpp
  ${RECLUSE_CORE_PUB_DIR}/Utility/Archive.hpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Utility/Archive.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Win32/Window.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Win32/Win32Time.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Win32/Keyboard.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Win32/Mouse.cpp
  ${RECLUSE_CORE_PUB_DIR}/Win32/Window.hpp
  ${RECLUSE_CORE_PUB_DIR}/Win32/Win32Configs.hpp
  ${RECLUSE_CORE_PUB_DIR}/Win32/Keyboard.hpp
  ${RECLUSE_CORE_PUB_DIR}/Win32/Mouse.hpp
  ${RECLUSE_CORE_PUB_DIR}/Serialize.hpp
  ${RECLUSE_CORE_PUB_DIR}/Core.hpp

  ${RECLUSE_CORE_PUB_DIR}/Logging/Log.hpp
  ${RECLUSE_CORE_PUB_DIR}/Exception.hpp

  ${RECLUSE_CORE_PRIVATE_DIR}/Core.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Logging/Log.cpp

  ${RECLUSE_CORE_PUB_DIR}/Thread/CoreThread.hpp
  ${RECLUSE_CORE_PUB_DIR}/Thread/Threading.hpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Utility/stb_image.hpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Utility/stb_image.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Utility/Image.cpp
  ${RECLUSE_CORE_PUB_DIR}/Types.hpp
  
  ${RECLUSE_CORE_PRIVATE_DIR}/Thread/CoreThread.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Thread/Threading.cpp

  ${RECLUSE_CORE_PUB_DIR}/Memory/SmartPointer.hpp
  ${RECLUSE_CORE_PUB_DIR}/Memory/Allocator.hpp
  ${RECLUSE_CORE_PUB_DIR}/Memory/StackAllocator.hpp
  ${RECLUSE_CORE_PUB_DIR}/Memory/PoolAllocator.hpp

  ${RECLUSE_CORE_PRIVATE_DIR}/Memory/StackAllocator.cpp
  ${RECLUSE_CORE_PRIVATE_DIR}/Memory/PoolAllocator.cpp
)


add_library(${RECLUSE_CORE} STATIC
  ${RECLUSE_CORE_FILES}
)

if (MSVC)
  foreach(source IN LISTS RECLUSE_CORE_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()

  set_property(TARGET ${RECLUSE_CORE}
    PROPERTY FOLDER ${RECLUSE_ENGINE}
  )
endif()