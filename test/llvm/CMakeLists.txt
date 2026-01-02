# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

include(${CMAKE_SOURCE_DIR}/cmake/Helper.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/llvm)

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
set_target_properties(JITLibraryNullTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(JITLibraryNullTest PRIVATE wasmedgeLLVM gtest gtest_main)
add_test(NAME JITLibraryNullTest COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/JITLibraryNullTest)
