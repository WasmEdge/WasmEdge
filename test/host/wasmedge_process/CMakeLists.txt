# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmedgeProcessTests
  wasmedge_process.cpp
)

target_link_libraries(wasmedgeProcessTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasmEdgeProcess
)

add_test(wasmedgeProcessTests wasmedgeProcessTests)
