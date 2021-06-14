# SPDX-License-Identifier: Apache-2.0

include(FetchContent)
FetchContent_Declare(
  wasmedge_unit_test
  GIT_REPOSITORY https://github.com/second-state/WasmEdge-unittest
  GIT_TAG wasm-dev-0.8.0
)
FetchContent_MakeAvailable(wasmedge_unit_test)

FetchContent_Declare(
  rapidjson
  GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "Build rapidjson documentation." FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Build rapidjson examples." FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Build rapidjson perftests and unittests." FORCE)
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "Build rapidjson with C++11" FORCE)
set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "Build rapidjson with C++17" FORCE)
FetchContent_MakeAvailable(rapidjson)
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ${rapidjson_SOURCE_DIR}/include)

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

target_link_libraries(wasmedgeTestSpec
  PRIVATE
  rapidjson
  PUBLIC
  std::filesystem
  wasmedgeCommon
  ${GTEST_BOTH_LIBRARIES}
)
