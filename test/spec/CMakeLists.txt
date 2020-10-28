# SPDX-License-Identifier: Apache-2.0

include(FetchContent)
FetchContent_Declare(
  ssvm_unit_test
  GIT_REPOSITORY https://github.com/second-state/ssvm-unittest.git
  GIT_TAG wasm-core
)

FetchContent_GetProperties(ssvm_unit_test)
if (NOT ssvm_unit_test_POPULATED)
    FetchContent_Populate(ssvm_unit_test)
endif ()

configure_files(
  ${ssvm_unit_test_SOURCE_DIR}/core/
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites/core/
  COPYONLY
)
configure_files(
  ${ssvm_unit_test_SOURCE_DIR}/reference-types/
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites/reference-types/
  COPYONLY
)
configure_files(
  ${ssvm_unit_test_SOURCE_DIR}/bulk-memory-operations/
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites/bulk-memory-operations/
  COPYONLY
)
configure_files(
  ${ssvm_unit_test_SOURCE_DIR}/simd/
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites/simd/
  COPYONLY
)

add_library(ssvmTestSpec
  spectest.cpp
)

target_include_directories(ssvmTestSpec
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(ssvmTestSpec
  PRIVATE
  std::filesystem
  utilGoogleTest
)
