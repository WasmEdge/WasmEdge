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

find_package(Python3 3.7 COMPONENTS Interpreter)
if(Python3_FOUND AND CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin|Windows)$"
   AND TARGET wasmedge AND TARGET wasmedgec)
  set(WASMEDGE_DEBUGGERS lldb)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(PREPEND WASMEDGE_DEBUGGERS gdb)
  endif()
  function(wasmedge_add_debugger_test DEBUGGER MODE OPT)
    string(TOUPPER "${DEBUGGER}_${MODE}" SUFFIX)
    if(NOT OPT STREQUAL "0")
      string(TOUPPER "${SUFFIX}_O${OPT}" SUFFIX)
    endif()
    set(TEST_NAME wasmedgeDebugInfo${SUFFIX})
    add_test(NAME ${TEST_NAME}
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/debuginfo/run_debugger_test.py
        --debugger ${DEBUGGER} --mode ${MODE} --opt ${OPT}
        --wasmedge $<TARGET_FILE:wasmedge>
        --wasmedgec $<TARGET_FILE:wasmedgec>
        --fixtures ${CMAKE_CURRENT_SOURCE_DIR}/debuginfo
        --helpers ${PROJECT_SOURCE_DIR}/utils/debug
    )
    set_tests_properties(${TEST_NAME} PROPERTIES SKIP_RETURN_CODE 77)
  endfunction()
  set(WASMEDGE_DEBUGGER_MODES aot jit lazyjit)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR
     (CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND LLVM_VERSION_MAJOR LESS 15))
    set(WASMEDGE_DEBUGGER_MODES aot)
  endif()
  foreach(DEBUGGER ${WASMEDGE_DEBUGGERS})
    foreach(MODE ${WASMEDGE_DEBUGGER_MODES})
      wasmedge_add_debugger_test(${DEBUGGER} ${MODE} 0)
    endforeach()
    wasmedge_add_debugger_test(${DEBUGGER} aot g)
  endforeach()
endif()
