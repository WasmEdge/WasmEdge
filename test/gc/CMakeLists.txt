# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeGCTests
  GCTest.cpp
)

add_test(wasmedgeGCTests wasmedgeGCTests)

target_link_libraries(wasmedgeGCTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
