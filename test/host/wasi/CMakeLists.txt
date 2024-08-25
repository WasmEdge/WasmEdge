# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasiTests
  wasi.cpp
  linuxTest.cpp
)

add_test(wasiTests wasiTests)

target_link_libraries(wasiTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasi
)

if(WASMEDGE_BUILD_COVERAGE)
  add_test(
    NAME wasi-test
    COMMAND bash -c "[ -e \"$HOME/.cargo/env\" ] && . \"$HOME/.cargo/env\"; \"${PROJECT_SOURCE_DIR}\"/utils/wasi-test/run-wasi-test.sh \"${PROJECT_BINARY_DIR}/tools/wasmedge\""
  )
endif()
