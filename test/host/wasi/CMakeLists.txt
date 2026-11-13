# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeHostWasiTests
  LinuxTest.cpp
  VfsIoTest.cpp
  WasiSocketTest.cpp
  WasiTest.cpp
)

add_test(wasmedgeHostWasiTests wasmedgeHostWasiTests)

target_link_libraries(wasmedgeHostWasiTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasi
  wasmedgeExecutor
  wasmedgeLoader
)

if(WIN32)
  target_link_libraries(wasmedgeHostWasiTests
    PRIVATE
    wsock32
    ws2_32
  )
endif()

if(WASMEDGE_BUILD_COVERAGE)
  add_test(
    NAME wasi-test
    COMMAND bash -c "[ -e \"$HOME/.cargo/env\" ] && . \"$HOME/.cargo/env\"; \"${PROJECT_SOURCE_DIR}\"/utils/wasi-test/run-wasi-test.sh \"${PROJECT_BINARY_DIR}/tools/wasmedge\""
  )
endif()
