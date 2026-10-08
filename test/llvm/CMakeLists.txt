# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeLLVMCoreTests
  LLVMcoreTest.cpp
  DebugInfoTest.cpp
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

find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}" "${LLVM_DIR}")
if(NOT LLVM_ENABLE_RTTI AND NOT MSVC)
  set_source_files_properties(DebugInfoTest.cpp
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

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/debuginfo
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND AND CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin|Windows)$"
   AND TARGET wasmedge AND TARGET wasmedgec)
  set(WASMEDGE_DEBUGGERS lldb)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(PREPEND WASMEDGE_DEBUGGERS gdb)
  endif()
  foreach(DEBUGGER ${WASMEDGE_DEBUGGERS})
    foreach(MODE aot jit)
      if(MODE STREQUAL "jit" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
        continue()
      endif()
      string(TOUPPER "${DEBUGGER}" DEBUGGER_UPPER)
      string(TOUPPER "${MODE}" MODE_UPPER)
      set(TEST_NAME wasmedgeDebugInfo${DEBUGGER_UPPER}_${MODE_UPPER})
      add_test(NAME ${TEST_NAME}
        COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/debuginfo/run_debugger_test.py
          --debugger ${DEBUGGER} --mode ${MODE}
          --wasmedge $<TARGET_FILE:wasmedge>
          --wasmedgec $<TARGET_FILE:wasmedgec>
          --fixtures ${CMAKE_CURRENT_BINARY_DIR}/debuginfo
          --helpers ${PROJECT_SOURCE_DIR}/utils/debug
      )
      set_tests_properties(${TEST_NAME} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
  endforeach()
  foreach(DEBUGGER ${WASMEDGE_DEBUGGERS})
    string(TOUPPER "${DEBUGGER}" DEBUGGER_UPPER)
    set(TEST_NAME wasmedgeDebugInfo${DEBUGGER_UPPER}_AOT_OG)
    add_test(NAME ${TEST_NAME}
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/debuginfo/run_debugger_test.py
        --debugger ${DEBUGGER} --mode aot --opt g
        --wasmedge $<TARGET_FILE:wasmedge>
        --wasmedgec $<TARGET_FILE:wasmedgec>
        --fixtures ${CMAKE_CURRENT_BINARY_DIR}/debuginfo
        --helpers ${PROJECT_SOURCE_DIR}/utils/debug
    )
    set_tests_properties(${TEST_NAME} PROPERTIES SKIP_RETURN_CODE 77)
  endforeach()
endif()
