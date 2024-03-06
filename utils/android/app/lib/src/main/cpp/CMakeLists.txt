cmake_minimum_required(VERSION 3.22.1)

project("wasmedge_lib")

set(WASMEDGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../..)
set(WASMEDGE_BUILD_PLUGINS OFF CACHE BOOL "" FORCE)
set(WASMEDGE_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(WASMEDGE_BUILD_SHARED_LIB ON CACHE BOOL "" FORCE)
set(WASMEDGE_USE_LLVM OFF CACHE BOOL "" FORCE)
set(WASMEDGE_FORCE_DISABLE_LTO ON CACHE BOOL "" FORCE)  # fast fixed problem for `-fuse-ld=gold`, we use lld.
if (CMAKE_GENERATOR STREQUAL Ninja)
  set(CMAKE_JOB_POOLS "link=2")
  set(CMAKE_JOB_POOL_LINK link)
endif()
add_subdirectory(${WASMEDGE_SOURCE_DIR} WasmEdge)
include_directories(${WasmEdge_BINARY_DIR}/include/api)

add_library(
        wasmedge_lib

        SHARED

        wasmedge_lib.cpp)



target_link_libraries(
        wasmedge_lib

        log
        wasmedge_shared
)
