# SPDX-License-Identifier: Apache-2.0

add_executable(wasmedgeProcessTests
  wasmedge_process.cpp
)

add_test(wasmedgeProcessTests wasmedgeProcessTests)

target_link_libraries(wasmedgeProcessTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAST
  wasmedgeHostModuleWasmEdgeProcess
)
