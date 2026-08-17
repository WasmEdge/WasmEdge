# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeGCTests
  GCTest.cpp
)

add_test(wasmedgeGCTests wasmedgeGCTests)

target_link_libraries(wasmedgeGCTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
