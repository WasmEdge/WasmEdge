# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeVMTests
  ThreadTest.cpp
  VMTest.cpp
)

add_test(wasmedgeVMTests wasmedgeVMTests)

target_link_libraries(wasmedgeVMTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)

if(WASMEDGE_USE_LLVM)
  target_compile_definitions(wasmedgeVMTests
    PRIVATE
    -DWASMEDGE_USE_LLVM
  )
  target_link_libraries(wasmedgeVMTests
    PRIVATE
    wasmedgeLLVM
  )
endif()
