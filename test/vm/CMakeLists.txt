# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeVMTests
  VMTest.cpp
)

add_test(wasmedgeVMTests wasmedgeVMTests)

target_link_libraries(wasmedgeVMTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
