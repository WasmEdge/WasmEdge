# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeAOTTests
  AOTBlake3Test.cpp
  AOTCacheTest.cpp
)

add_test(wasmedgeAOTTests wasmedgeAOTTests)

target_link_libraries(wasmedgeAOTTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAOT
)

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/mixcallTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_executable(wasmedgeLLVMTests
  LazyJITTest.cpp
  LLVMInterruptTest.cpp
  LLVMRegressionTest.cpp
  MixCallTest.cpp
)

add_test(wasmedgeLLVMTests wasmedgeLLVMTests)

target_link_libraries(wasmedgeLLVMTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)
