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
  # "wasm-3.0-memory64"
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

# Download component-model test suite
message(STATUS "Downloading the Component Model spec test suite")
FetchContent_Declare(
  wasmedge_component_model_test
  GIT_REPOSITORY https://github.com/WebAssembly/component-model.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(wasmedge_component_model_test)
message(STATUS "Downloading the Component Model spec test suite -- done")

# Copy component-model validator tests
message(STATUS "Copying test suite to ${CMAKE_CURRENT_BINARY_DIR}/testSuites/component-model")
file(COPY ${wasmedge_component_model_test_SOURCE_DIR}/test/wasmtime/
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/testSuites/component-model
     FILES_MATCHING PATTERN "*.wast" PATTERN "*.json")
message(STATUS "Copying test suite to ${CMAKE_CURRENT_BINARY_DIR}/testSuites/component-model -- done")

# Component Model validator test executable
wasmedge_add_executable(wasmedgeComponentModelTest
  componentModelTest.cpp
)

target_include_directories(wasmedgeComponentModelTest
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgeComponentModelTest
  PRIVATE
  wasmedgeCommon
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
)

# Add CTest integration
add_test(NAME ComponentModelTest COMMAND wasmedgeComponentModelTest)

# Note: Tests are placeholder checks until validator is complete
# Once validator implementation progresses, expand tests to validate component model spec
