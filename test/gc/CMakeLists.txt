# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeGCTests
  GCTest.cpp
)

add_test(wasmedgeGCTests wasmedgeGCTests)

target_link_libraries(wasmedgeGCTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)

# The capability-gate tests drive the manual Loader/Compiler/JIT path, so they
# are compiled only when the AOT compiler is built.
if(WASMEDGE_USE_LLVM)
  target_compile_definitions(wasmedgeGCTests
    PRIVATE
    -DWASMEDGE_USE_LLVM
  )
  target_link_libraries(wasmedgeGCTests
    PRIVATE
    wasmedgeLLVM
  )
endif()
