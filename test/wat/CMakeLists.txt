# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

wasmedge_add_executable(wasmedgeWatTests
  maybeWATTest.cpp
)

add_test(wasmedgeWatTests wasmedgeWatTests)

target_link_libraries(wasmedgeWatTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeWat
)
