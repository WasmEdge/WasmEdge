# SPDX-License-Identifier: Apache-2.0

add_executable(poTests
  gtest.cpp
  po.cpp
)

add_test(poTests poTests)

target_link_libraries(poTests
  PRIVATE
  utilGoogleTest
)

target_include_directories(poTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
