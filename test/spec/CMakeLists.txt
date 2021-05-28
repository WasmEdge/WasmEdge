# SPDX-License-Identifier: Apache-2.0

include(FetchContent)
FetchContent_Declare(
  wasmedge_unit_test
  GIT_REPOSITORY https://github.com/second-state/WasmEdge-unittest
  GIT_TAG wasm-dev-0.8.0
)

FetchContent_GetProperties(wasmedge_unit_test)
if (NOT wasmedge_unit_test_POPULATED)
    FetchContent_Populate(wasmedge_unit_test)
endif ()

configure_files(
  ${wasmedge_unit_test_SOURCE_DIR}/core/
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites/core/
  COPYONLY
)
configure_files(
  ${wasmedge_unit_test_SOURCE_DIR}/simd/
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites/simd/
  COPYONLY
)

add_library(wasmedgeTestSpec
  spectest.cpp
)

target_include_directories(wasmedgeTestSpec
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeTestSpec
  PRIVATE
  std::filesystem
  wasmedgeCommon
  ${GTEST_BOTH_LIBRARIES}
)
