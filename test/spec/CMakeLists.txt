# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

include(FetchContent)
FetchContent_Declare(
  wasmedge_unit_test
  GIT_REPOSITORY https://github.com/second-state/WasmEdge-unittest
  GIT_TAG        wasm-dev
)
FetchContent_MakeAvailable(wasmedge_unit_test)

FetchContent_Declare(
  rapidjson
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
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "Build rapidjson documentation." FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Build rapidjson examples." FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Build rapidjson perftests and unittests." FORCE)
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "Build rapidjson with C++11" FORCE)
set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "Build rapidjson with C++17" FORCE)
FetchContent_MakeAvailable(rapidjson)
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ${rapidjson_SOURCE_DIR}/include)

file(COPY
  ${wasmedge_unit_test_SOURCE_DIR}/core
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites
)
file(COPY
  ${wasmedge_unit_test_SOURCE_DIR}/multi-memory
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites
)
file(COPY
  ${wasmedge_unit_test_SOURCE_DIR}/tail-call
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites
)

wasmedge_add_library(wasmedgeTestSpec
  spectest.cpp
)

target_link_libraries(wasmedgeTestSpec
  PRIVATE
  rapidjson
  PUBLIC
  std::filesystem
  wasmedgeCommon
  ${GTEST_BOTH_LIBRARIES}
)
