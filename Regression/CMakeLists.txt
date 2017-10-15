cmake_minimum_required(VERSION 3.0)
project("Regressions")


set(REGRESSIONS_NAME "Regression")

set(REGRESSIONS_FILES
  Main.cpp
)


add_executable(${REGRESSIONS_NAME}
  ${REGRESSIONS_FILES}
)