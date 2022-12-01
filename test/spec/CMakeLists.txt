# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

include(FetchContent)

message(STATUS "Downloading the WASM spec test suite")
FetchContent_Declare(
  wasmedge_unit_test
  GIT_REPOSITORY https://github.com/second-state/WasmEdge-unittest
  GIT_TAG        wasm-dev
)
FetchContent_MakeAvailable(wasmedge_unit_test)
message(STATUS "Downloading the WASM spec test suite -- done")

find_package(RapidJSON QUIET)
if(NOT RapidJSON_FOUND)
  message(STATUS "Downloading RapidJSON source")
  FetchContent_Declare(
    RapidJSON
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    )

  get_property(
    compile_options
    DIRECTORY
    PROPERTY COMPILE_OPTIONS
    )
  set_property(
    DIRECTORY
    APPEND
    PROPERTY COMPILE_OPTIONS
    -Wno-conditional-uninitialized
    -Wno-implicit-int-conversion
    -Wno-zero-as-null-pointer-constant
    )
  unset(compile_options)
  set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "Build RapidJSON documentation." FORCE)
  set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Build RapidJSON examples." FORCE)
  set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Build RapidJSON perftests and unittests." FORCE)
  set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "Build RapidJSON with C++11" FORCE)
  set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "Build RapidJSON with C++17" FORCE)
  FetchContent_MakeAvailable(RapidJSON)
  message(STATUS "Downloading RapidJSON source -- done")
endif()

add_library(RapidJSON INTERFACE)
if(RAPIDJSON_INCLUDE_DIRS)
  target_include_directories(RapidJSON INTERFACE ${RAPIDJSON_INCLUDE_DIRS})
else()
  target_include_directories(RapidJSON INTERFACE ${RapidJSON_SOURCE_DIR}/include)
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
  RapidJSON
  PUBLIC
  std::filesystem
  wasmedgeCommon
  ${GTEST_BOTH_LIBRARIES}
)
