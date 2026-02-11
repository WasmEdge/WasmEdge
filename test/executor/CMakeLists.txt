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

wasmedge_add_executable(wasmedgeExecutorNullDerefTests
  executorNullDerefTest.cpp
)

add_test(wasmedgeExecutorNullDerefTests wasmedgeExecutorNullDerefTests)

target_link_libraries(wasmedgeExecutorNullDerefTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
