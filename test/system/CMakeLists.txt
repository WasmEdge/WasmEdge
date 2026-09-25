# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeSystemTests
  allocatorTest.cpp
  mmapTest.cpp
  pathTest.cpp
)

add_test(wasmedgeSystemTests wasmedgeSystemTests)

target_link_libraries(wasmedgeSystemTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeSystem
)
