# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_setup_simdjson()

wasmedge_add_executable(wasmedgeComponentRegressionTests
  ComponentRegressionTest.cpp
)

add_test(wasmedgeComponentRegressionTests wasmedgeComponentRegressionTests)

target_link_libraries(wasmedgeComponentRegressionTests
  PRIVATE
  wasmedgeVM
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(wasmedgeComponentRegressionTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

add_subdirectory(fuzzing)
