# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

wasmedge_add_executable(wasmedgeMemInstanceTests
  MemInstanceTest.cpp
)

add_test(wasmedgeMemInstanceTests wasmedgeMemInstanceTests)

target_link_libraries(wasmedgeMemInstanceTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
