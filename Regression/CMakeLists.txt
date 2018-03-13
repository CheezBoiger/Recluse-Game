cmake_minimum_required(VERSION 3.0)
project("Regressions")

set(REGRESSIONS_NAME "Regression")
include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
)

set(REGRESSIONS_MATH_FILES
  Math/TestMath.hpp
  Math/TestMatrix.cpp
  Math/TestQuaternion.cpp
  Math/TestRay.cpp
  Math/TestVector.cpp

  Game/TestGameObject.hpp
  Game/TestGameObject.cpp
  Game/TestGameObjectManager.cpp

  Memory/TestMemory.hpp
  Memory/TestAllocator.cpp
)

set(REGRESSIONS_FILES
  Tester.hpp
  Main.cpp
  ${REGRESSIONS_MATH_FILES}
)

# Force static runtime libraries
foreach(flag
CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG_INIT
CMAKE_CXX_FLAGS_RELEASE  CMAKE_CXX_FLAGS_RELWITHDEBINFO
CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
  string(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
  set("${flag}" "${${flag}} /EHsc")
endforeach()


add_executable(${REGRESSIONS_NAME}
  ${REGRESSIONS_FILES}
)


target_link_libraries(${REGRESSIONS_NAME} ${RECLUSE_ENGINE_LINK_LIBRARIES})
copy_engine_dependencies_to_exe(${REGRESSIONS_NAME})