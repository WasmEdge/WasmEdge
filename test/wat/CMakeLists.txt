# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeWatTests
  maybeWATTest.cpp
  converterTest.cpp
)

add_test(wasmedgeWatTests wasmedgeWatTests)

target_link_libraries(wasmedgeWatTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeWat
)
