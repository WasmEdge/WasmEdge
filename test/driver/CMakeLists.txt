# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeDriverToolTests
  driverToolTest.cpp
)

add_test(NAME wasmedgeDriverToolTests
  COMMAND wasmedgeDriverToolTests
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeDriverToolTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeDriver
)
