# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeHostMockTests
  MockTest.cpp
)

add_test(wasmedgeHostMockTests wasmedgeHostMockTests)

target_link_libraries(wasmedgeHostMockTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
