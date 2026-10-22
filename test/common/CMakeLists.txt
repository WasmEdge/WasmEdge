# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeCommonTests
  ByteswapTest.cpp
  DenseEnumMapTest.cpp
  ErrInfoTest.cpp
  ExpectedAssignmentTest.cpp
  ExpectedBasesTest.cpp
  ExpectedConstexprTest.cpp
  ExpectedConstructorsTest.cpp
  ExpectedEmplaceTest.cpp
  ExpectedExtensionsTest.cpp
  ExpectedIssuesTest.cpp
  ExpectedNoexceptTest.cpp
  ExpectedObserversTest.cpp
  ExpectedRelopsTest.cpp
  ExpectedSwapTest.cpp
  HashTest.cpp
  HexStrTest.cpp
  Int128Test.cpp
  RoundevenTest.cpp
  SpanTest.cpp
  SpareEnumMapTest.cpp
  SpdlogTest.cpp
  StatisticsTest.cpp
)

add_test(wasmedgeCommonTests wasmedgeCommonTests)

target_link_libraries(wasmedgeCommonTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeCommon
)
