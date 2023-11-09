# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

include(FetchContent)

wasmedge_setup_simdjson()

message(STATUS "Downloading the WASM spec test suite")
FetchContent_Declare(
  wasmedge_unit_test
  GIT_REPOSITORY https://github.com/WasmEdge/wasmedge-spectest
  GIT_TAG        wasm-core-20251029
)
FetchContent_MakeAvailable(wasmedge_unit_test)
message(STATUS "Downloading the WASM spec test suite -- done")

function(wasmedge_copy_spec_testsuite proposal)
  message(STATUS "Copying test suite to ${CMAKE_CURRENT_BINARY_DIR}/testSuites/${proposal}")
  file(COPY
    ${wasmedge_unit_test_SOURCE_DIR}/${proposal}
    DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}/testSuites
  )
  message(STATUS "Copying test suite to ${CMAKE_CURRENT_BINARY_DIR}/testSuites/${proposal} -- done")
endfunction()

set(WASMEDGE_SPECTEST_FOLDERS
  "wasm-1.0"
  "wasm-2.0"
  "wasm-3.0"
  "wasm-3.0-bulk-memory"
  "wasm-3.0-exceptions"
  "wasm-3.0-gc"
  "wasm-3.0-memory64"
  "wasm-3.0-multi-memory"
  "wasm-3.0-relaxed-simd"
  "wasm-3.0-simd"
  "threads"
)

foreach(PROPOSAL ${WASMEDGE_SPECTEST_FOLDERS})
  wasmedge_copy_spec_testsuite(${PROPOSAL})
endforeach()

wasmedge_add_library(wasmedgeTestSpec
  spectest.cpp
)

target_link_libraries(wasmedgeTestSpec
  PRIVATE
  simdjson::simdjson
  PUBLIC
  std::filesystem
  wasmedgeCommon
  ${GTEST_BOTH_LIBRARIES}
)
