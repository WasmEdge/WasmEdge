# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeLLVMCoreTests
  LLVMcoreTest.cpp
)

add_test(wasmedgeLLVMCoreTests wasmedgeLLVMCoreTests)

target_link_libraries(wasmedgeLLVMCoreTests
  PRIVATE
  wasmedgeTestSpec
  wasmedgeVM
)

# The write-barrier test walks the compiled LLVM module through the LLVM C API,
# so it needs lib/llvm (for Data::DataContext) and the LLVM headers. Both are
# private to wasmedgeLLVM; borrow them rather than making them public.
target_include_directories(wasmedgeLLVMCoreTests
  SYSTEM PRIVATE
  $<TARGET_PROPERTY:wasmedgeLLVM,INCLUDE_DIRECTORIES>
)

wasmedge_add_executable(wasmedgeLazyJITTests
  LazyJITTest.cpp
)

add_test(wasmedgeLazyJITTests wasmedgeLazyJITTests)

target_link_libraries(wasmedgeLazyJITTests
  PRIVATE
  wasmedgeVM
  GTest::gtest
)
