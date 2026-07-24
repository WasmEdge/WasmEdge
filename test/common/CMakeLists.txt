# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeCommonTests
  int128Test.cpp
  hashTest.cpp
  spdlogTest.cpp
  roundevenTest.cpp
  hexstrTest.cpp
  spareEnumMapTest.cpp
  byteswapTest.cpp
  statisticsTest.cpp
  denseEnumMapTest.cpp
)

add_test(wasmedgeCommonTests wasmedgeCommonTests)

target_link_libraries(wasmedgeCommonTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeCommon
)
