# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeHostMockTests
  mockTest.cpp
)

add_test(wasmedgeHostMockTests wasmedgeHostMockTests)

target_link_libraries(wasmedgeHostMockTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
