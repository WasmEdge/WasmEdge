# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeThreadTests
  ThreadTest.cpp
)

add_test(wasmedgeThreadTests wasmedgeThreadTests)

target_link_libraries(wasmedgeThreadTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)

if(WASMEDGE_USE_LLVM)
  target_compile_definitions(wasmedgeThreadTests
    PRIVATE
    -DWASMEDGE_USE_LLVM
  )
  target_link_libraries(wasmedgeThreadTests
    PRIVATE
    wasmedgeLLVM
  )
endif()
