# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(poTests
  gtest.cpp
  po.cpp
  help.cpp
  subcommand.cpp
)

add_test(poTests poTests)

target_link_libraries(poTests
  PRIVATE
  wasmedgePO
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(poTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
