# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeDriverToolTests
  driverToolTest.cpp
)

add_test(NAME wasmedgeDriverToolTests
  COMMAND wasmedgeDriverToolTests
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeDriverToolTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeDriver
)

if(WASMEDGE_USE_LLVM)
  find_package(LLVM QUIET HINTS "${LLVM_CMAKE_PATH}" "${LLVM_DIR}")
  if(LLVM_FOUND)
    target_compile_definitions(wasmedgeDriverToolTests
      PRIVATE
      WASMEDGE_LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}
    )
  endif()
endif()
