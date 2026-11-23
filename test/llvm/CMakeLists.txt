# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeLLVMCoreTests
  LLVMcoreTest.cpp
  DebugInfoReaderTest.cpp
  DebugInfoTypesTest.cpp
  DebugInfoAOTTest.cpp
  DebugInfoRunTest.cpp
)

add_test(wasmedgeLLVMCoreTests wasmedgeLLVMCoreTests)

target_link_libraries(wasmedgeLLVMCoreTests
  PRIVATE
  wasmedgeTestSpec
  wasmedgeVM
)

target_include_directories(wasmedgeLLVMCoreTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/lib/llvm
)

target_compile_definitions(wasmedgeLLVMCoreTests
  PRIVATE
  WASMEDGE_DEBUGINFO_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/debuginfo"
)

find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}" "${LLVM_DIR}")
if(NOT LLVM_ENABLE_RTTI AND NOT MSVC)
  set_source_files_properties(
    DebugInfoReaderTest.cpp
    DebugInfoTypesTest.cpp
    DebugInfoAOTTest.cpp
    DebugInfoRunTest.cpp
    PROPERTIES COMPILE_OPTIONS -fno-rtti)
endif()
target_include_directories(wasmedgeLLVMCoreTests
  SYSTEM
  PRIVATE
  ${LLVM_INCLUDE_DIRS}
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
