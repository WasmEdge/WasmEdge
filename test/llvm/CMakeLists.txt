# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeLLVMCoreTests
  LLVMcoreTest.cpp
)

add_test(wasmedgeLLVMCoreTests wasmedgeLLVMCoreTests)

target_link_libraries(wasmedgeLLVMCoreTests
  PRIVATE
  wasmedgeTestSpec
  wasmedgeVM
)

add_executable(JITLibraryNullTest JITLibraryNullTest.cpp)
target_link_libraries(JITLibraryNullTest PRIVATE wasmedgeLLVM gtest)
add_test(NAME JITLibraryNullTest COMMAND JITLibraryNullTest)
