# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgePOTests
  HelpTest.cpp
  OptionTest.cpp
  SubcommandTest.cpp
)

add_test(wasmedgePOTests wasmedgePOTests)

target_link_libraries(wasmedgePOTests
  PRIVATE
  wasmedgePO
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(wasmedgePOTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
