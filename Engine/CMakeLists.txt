cmake_minimum_required(VERSION 3.0)
project("Engine")

# Force static runtime libraries
foreach(flag
CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG_INIT
CMAKE_CXX_FLAGS_RELEASE  CMAKE_CXX_FLAGS_RELWITHDEBINFO
CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
  string(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
  set("${flag}" "${${flag}} /EHsc")
endforeach()


add_subdirectory(Renderer)
add_subdirectory(UI)
add_subdirectory(Game)
add_subdirectory(Filesystem)
add_subdirectory(Core)
add_subdirectory(Animation)

if (PHYSICS)
  add_subdirectory(Physics)
endif()

if (AUDIO)
  if (WIN32)
    # Enable unicode api calling for WWise.
    add_definitions(-DUNICODE=1)
  endif()
  add_subdirectory(Audio)
endif()