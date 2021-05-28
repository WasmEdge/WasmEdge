# SPDX-License-Identifier: Apache-2.0

add_executable(wasmedgeInterpreterCoreTests
  InterpreterTest.cpp
)

add_test(wasmedgeInterpreterCoreTests wasmedgeInterpreterCoreTests)

target_link_libraries(wasmedgeInterpreterCoreTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeTestSpec
  wasmedgeVM
)
