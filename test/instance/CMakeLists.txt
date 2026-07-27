# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

wasmedge_add_executable(wasmedgeRuntimeInstanceTests
  MemInstanceTest.cpp
  ModuleInstanceTest.cpp
  RefLifetimeTest.cpp
)

add_test(wasmedgeRuntimeInstanceTests wasmedgeRuntimeInstanceTests)

target_link_libraries(wasmedgeRuntimeInstanceTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
  Threads::Threads
)
