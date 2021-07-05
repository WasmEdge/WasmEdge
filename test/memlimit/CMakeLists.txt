# SPDX-License-Identifier: Apache-2.0

wasmedge_add_executable(wasmedgeMemLimitTests
  MemLimitTest.cpp
)

add_test(wasmedgeMemLimitTests wasmedgeMemLimitTests)

target_link_libraries(wasmedgeMemLimitTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
