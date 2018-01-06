cmake_minimum_required(VERSION 3.0)
project("Testing")

set(RECLUSE_TEST  "Test")
set(RECLUSE_GAME_TEST "GameTest")

# Force static runtime libraries
foreach(flag
CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG_INIT
CMAKE_CXX_FLAGS_RELEASE  CMAKE_CXX_FLAGS_RELWITHDEBINFO
CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
  string(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
  set("${flag}" "${${flag}} /EHsc")
endforeach()

set(RECLUSE_TEST_FILES
  Main.cpp
)

set(RECLUSE_GAME_TEST_FILES Game/Main.cpp)

add_executable(${RECLUSE_TEST}
  ${RECLUSE_TEST_FILES}
)

add_executable(${RECLUSE_GAME_TEST}
  ${RECLUSE_GAME_TEST_FILES}
)

include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
)

target_link_libraries(${RECLUSE_TEST}
  ${RECLUSE_ENGINE_LINK_LIBRARIES}
)

target_link_libraries(${RECLUSE_GAME_TEST}
  ${RECLUSE_ENGINE_LINK_LIBRARIES}
)