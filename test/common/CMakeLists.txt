# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeCommonTests
  int128Test.cpp
)

add_test(wasmedgeCommonTests wasmedgeCommonTests)

target_link_libraries(wasmedgeCommonTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeCommon
)
