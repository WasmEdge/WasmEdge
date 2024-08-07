# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasiSocketTests
  wasi_socket.cpp
)

add_test(wasiSocketTests wasiSocketTests)

target_link_libraries(wasiSocketTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasi
  wasmedgeExecutor
)

if(WIN32)
  target_link_libraries(wasiSocketTests
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
