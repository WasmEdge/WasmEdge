# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

wasmedge_add_executable(wasmedgeModuleInstanceTests
  ModuleInstanceTest.cpp
)

add_test(wasmedgeModuleInstanceTests wasmedgeModuleInstanceTests)

target_link_libraries(wasmedgeModuleInstanceTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
