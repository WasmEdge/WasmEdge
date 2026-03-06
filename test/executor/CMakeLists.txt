# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeExecutorCoreTests
  ExecutorTest.cpp
)

add_test(wasmedgeExecutorCoreTests wasmedgeExecutorCoreTests)

target_link_libraries(wasmedgeExecutorCoreTests
  PRIVATE
  wasmedgeTestSpec
  wasmedgeVM
)

wasmedge_add_executable(wasmedgeExecutorRegressionTests
  ExecutorRegressionTest.cpp
)

add_test(wasmedgeExecutorRegressionTests
  wasmedgeExecutorRegressionTests
)

target_link_libraries(wasmedgeExecutorRegressionTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
