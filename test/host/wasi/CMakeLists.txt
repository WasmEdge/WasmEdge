# SPDX-License-Identifier: Apache-2.0

add_executable(wasiTests
  wasi.cpp
)

add_test(wasiTests wasiTests)

target_link_libraries(wasiTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAST
  wasmedgeHostModuleWasi
)
