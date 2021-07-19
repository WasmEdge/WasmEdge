# SPDX-License-Identifier: Apache-2.0

add_executable(wasmedgeAOTCoreTests
  AOTcoreTest.cpp
)

add_test(wasmedgeAOTCoreTests wasmedgeAOTCoreTests)

target_compile_options(wasmedgeAOTCoreTests
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeAOTCoreTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeTestSpec
  wasmedgeLoader
  wasmedgeAOT
  wasmedgeVM
)

add_executable(wasmedgeAOTCacheTests
  AOTCacheTest.cpp
)

add_test(wasmedgeAOTCacheTests wasmedgeAOTCacheTests)

target_compile_options(wasmedgeAOTCacheTests
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeAOTCacheTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAOT
)

add_executable(wasmedgeAOTBlake3Tests
  AOTBlake3Test.cpp
)

add_test(wasmedgeAOTBlake3Tests wasmedgeAOTBlake3Tests)

target_compile_options(wasmedgeAOTBlake3Tests
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeAOTBlake3Tests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAOT
)
