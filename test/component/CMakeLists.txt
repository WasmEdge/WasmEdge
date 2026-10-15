# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_setup_simdjson()

wasmedge_add_executable(wasmedgeComponentTests
  ComponentRegressionTest.cpp
  ComponentLoaderTest.cpp
  ComponentValidatorTest.cpp
  ComponentCanonABITest.cpp
  ComponentCanonInvokeTest.cpp
)

add_test(wasmedgeComponentTests wasmedgeComponentTests)

target_link_libraries(wasmedgeComponentTests
  PRIVATE
  wasmedgeVM
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(wasmedgeComponentTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
