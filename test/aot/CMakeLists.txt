# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmedgeAOTCoreTests
  AOTcoreTest.cpp
)

add_test(wasmedgeAOTCoreTests wasmedgeAOTCoreTests)

target_link_libraries(wasmedgeAOTCoreTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeTestSpec
  wasmedgeLoader
  wasmedgeAOT
  wasmedgeVM
)

wasmedge_add_executable(wasmedgeAOTCacheTests
  AOTCacheTest.cpp
)

add_test(wasmedgeAOTCacheTests wasmedgeAOTCacheTests)

target_link_libraries(wasmedgeAOTCacheTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAOT
)

wasmedge_add_executable(wasmedgeAOTBlake3Tests
  AOTBlake3Test.cpp
)

add_test(wasmedgeAOTBlake3Tests wasmedgeAOTBlake3Tests)

target_link_libraries(wasmedgeAOTBlake3Tests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAOT
)
