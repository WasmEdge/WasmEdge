# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeExecutorTests
  CoredumpTest.cpp
  ExecutorInterruptTest.cpp
  ExecutorRegressionTest.cpp
)

add_test(wasmedgeExecutorTests wasmedgeExecutorTests)

target_link_libraries(wasmedgeExecutorTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
