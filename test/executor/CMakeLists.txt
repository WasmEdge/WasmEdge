# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeExecutorCoreTests
  ExecutorTest.cpp
)

add_test(wasmedgeExecutorCoreTests wasmedgeExecutorCoreTests)

target_link_libraries(wasmedgeExecutorCoreTests
  PRIVATE
  wasmedgeTestSpec
  wasmedgeVM
)

wasmedge_add_executable(wasmedgeExecutorRegressionTests
  ExecutorRegressionTest.cpp
)

add_test(wasmedgeExecutorRegressionTests
  wasmedgeExecutorRegressionTests
)

target_link_libraries(wasmedgeExecutorRegressionTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)

wasmedge_add_executable(wasmedgeConstFoldTests
  ConstFoldTest.cpp
)

add_test(wasmedgeConstFoldTests wasmedgeConstFoldTests)

target_link_libraries(wasmedgeConstFoldTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeExecutor
  wasmedgeCommon
)

wasmedge_add_executable(wasmedgeIPCPEdgeCaseTests
  IPCPEdgeCaseTest.cpp
)

add_test(wasmedgeIPCPEdgeCaseTests wasmedgeIPCPEdgeCaseTests)

target_link_libraries(wasmedgeIPCPEdgeCaseTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoader
  wasmedgeValidator
  wasmedgeExecutor
  wasmedgeCommon
)

# The const-fold benchmark loads .wasm files at runtime; compile them from
# .wat sources via wat2wasm at build time so no pre-built binaries live in
# the repo. If wat2wasm isn't installed, the benchmark target is skipped.
find_program(WAT2WASM_EXECUTABLE wat2wasm)
if(WAT2WASM_EXECUTABLE)
  set(_const_fold_bench_wasm_files)
  foreach(WAT_NAME
      fibonacci_bench
      factorial_bench
      mandelbrot_bench
      mandelbrot_c
      constFoldBench)
    set(_wat_src ${CMAKE_CURRENT_SOURCE_DIR}/${WAT_NAME}.wat)
    set(_wasm_out ${CMAKE_CURRENT_BINARY_DIR}/${WAT_NAME}.wasm)
    add_custom_command(
      OUTPUT ${_wasm_out}
      COMMAND ${WAT2WASM_EXECUTABLE} ${_wat_src} -o ${_wasm_out}
      DEPENDS ${_wat_src}
      COMMENT "wat2wasm ${WAT_NAME}.wat"
      VERBATIM)
    list(APPEND _const_fold_bench_wasm_files ${_wasm_out})
  endforeach()

  add_custom_target(wasmedgeConstFoldBenchmarkFixtures
    DEPENDS ${_const_fold_bench_wasm_files})

  wasmedge_add_executable(wasmedgeConstFoldBenchmark
    ConstFoldBenchmark.cpp
  )
  add_dependencies(wasmedgeConstFoldBenchmark wasmedgeConstFoldBenchmarkFixtures)
  target_link_libraries(wasmedgeConstFoldBenchmark
    PRIVATE
    wasmedgeVM
    wasmedgeLoader
    wasmedgeValidator
    wasmedgeExecutor
    wasmedgeCommon
  )
else()
  message(STATUS "wat2wasm not found; skipping wasmedgeConstFoldBenchmark target")
endif()
