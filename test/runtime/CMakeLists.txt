# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeRuntimeTests
  MemInstanceTest.cpp
  ModuleInstanceTest.cpp
  RefLifetimeTest.cpp
)

add_test(wasmedgeRuntimeTests wasmedgeRuntimeTests)

target_link_libraries(wasmedgeRuntimeTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
  Threads::Threads
)
