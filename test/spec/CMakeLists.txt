# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

include(FetchContent)

message(STATUS "Downloading the WASM spec test suite")
FetchContent_Declare(
  wasmedge_unit_test
  GIT_REPOSITORY https://github.com/second-state/WasmEdge-unittest
  GIT_TAG        wasm-dev-0.13.0
)
FetchContent_MakeAvailable(wasmedge_unit_test)
message(STATUS "Downloading the WASM spec test suite -- done")

find_package(simdjson QUIET)
if(simdjson_FOUND)
  message(STATUS "SIMDJSON found")
else()
  message(STATUS "Downloading SIMDJSON source")
  include(FetchContent)
  FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG  tags/v3.2.1
    GIT_SHALLOW TRUE)

  if(MSVC)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      get_property(
        compile_options
        DIRECTORY
        PROPERTY COMPILE_OPTIONS
        )
      set_property(
        DIRECTORY
        APPEND
        PROPERTY COMPILE_OPTIONS
        -Wno-undef
        -Wno-suggest-override
        -Wno-documentation
        -Wno-sign-conversion
        -Wno-extra-semi-stmt
        -Wno-old-style-cast
        -Wno-error=unused-parameter
        -Wno-error=unused-template
        -Wno-conditional-uninitialized
        -Wno-implicit-int-conversion
        -Wno-shorten-64-to-32
        -Wno-range-loop-bind-reference
        -Wno-format-nonliteral
        -Wno-unused-exception-parameter
        -Wno-unused-member-function
        )
      unset(compile_options)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      set_property(
        DIRECTORY
        APPEND
        PROPERTY COMPILE_OPTIONS
        /wd4100 # unreferenced formal parameter
        )
    endif()
  endif()

  FetchContent_MakeAvailable(simdjson)

  set(SIMDJSON_JUST_LIBRARY ON CACHE INTERNAL "")
  set(SIMDJSON_BUILD_STATIC ON CACHE INTERNAL "")

  message(STATUS "Downloading SIMDJSON source -- done")
endif()

function(wasmedge_copy_spec_testsuite proposal)
  message(STATUS "Copying test suite to ${CMAKE_CURRENT_BINARY_DIR}/testSuites/${proposal}")
  file(COPY
    ${wasmedge_unit_test_SOURCE_DIR}/${proposal}
    DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}/testSuites
  )
  message(STATUS "Copying test suite to ${CMAKE_CURRENT_BINARY_DIR}/testSuites/${proposal} -- done")
endfunction()

foreach(PROPOSAL core multi-memory tail-call extended-const threads)
  wasmedge_copy_spec_testsuite(${PROPOSAL})
endforeach()

wasmedge_add_library(wasmedgeTestSpec
  spectest.cpp
)

target_link_libraries(wasmedgeTestSpec
  PRIVATE
  simdjson
  PUBLIC
  std::filesystem
  wasmedgeCommon
  ${GTEST_BOTH_LIBRARIES}
)
