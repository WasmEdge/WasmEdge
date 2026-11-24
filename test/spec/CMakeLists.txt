# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include(FetchContent)

wasmedge_setup_simdjson()

message(STATUS "Downloading the WASM spec test suite")
FetchContent_Declare(
  wasmedge_unit_test
  GIT_REPOSITORY https://github.com/WasmEdge/wasmedge-spectest
  GIT_TAG        0cb703e9402f0886449fc9a822b64b89ed4b87a4  # wasm-core-20261123
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
  "wide-arithmetic"
  "component-model-async"
  "component-model-binary"
  "component-model-linking"
  "component-model-resources"
  "component-model-validation"
  "component-model-values"
)

foreach(PROPOSAL ${WASMEDGE_SPECTEST_FOLDERS})
  wasmedge_copy_spec_testsuite(${PROPOSAL})
endforeach()

set(WASMEDGE_TESTSPEC_SOURCES
  spectest.cpp
  json_parser.cpp
)
if(WASMEDGE_BUILD_WAT)
  list(APPEND WASMEDGE_TESTSPEC_SOURCES wast_parser.cpp)
endif()

wasmedge_add_library(wasmedgeTestSpec
  ${WASMEDGE_TESTSPEC_SOURCES}
)

target_link_libraries(wasmedgeTestSpec
  PRIVATE
  simdjson::simdjson
  wasmedgeLoader
  PUBLIC
  std::filesystem
  wasmedgeCommon
  ${GTEST_BOTH_LIBRARIES}
)

# The WAST runner and the tests of the WAT loader need wasmedgeWat. A test
# that links wasmedgeTestSpec gets the WASMEDGE_BUILD_WAT macro with it.
if(WASMEDGE_BUILD_WAT)
  target_link_libraries(wasmedgeTestSpec
    PUBLIC
    wasmedgeWat
  )

  add_executable(wast2wat EXCLUDE_FROM_ALL
    wast2wat.cpp
    wast_parser.cpp
  )
  wasmedge_setup_target(wast2wat)

  target_link_libraries(wast2wat
    PRIVATE
    wasmedgeWat
    wasmedgeCommon
    std::filesystem
  )
endif()
