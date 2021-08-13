# SPDX-License-Identifier: Apache-2.0

wasmedge_add_executable(poTests
  gtest.cpp
  po.cpp
  help.cpp
  subcommand.cpp
)

add_test(poTests poTests)

target_link_libraries(poTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(poTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
