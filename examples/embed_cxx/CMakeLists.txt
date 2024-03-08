cmake_minimum_required(VERSION 3.16)

project(embed_cxx CXX)

set(CMAKE_CXX_STANDARD 17)

set(WASMEDGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(WASMEDGE_USE_LLVM OFF)
if (CMAKE_GENERATOR STREQUAL Ninja)
  set(CMAKE_JOB_POOLS "link=2")
  set(CMAKE_JOB_POOL_LINK link)
endif()
add_subdirectory(${WASMEDGE_SOURCE_DIR} WasmEdge)
include_directories(${WasmEdge_BINARY_DIR}/include/api)

add_subdirectory(script)

add_executable(
    embed_cxx
    main.cpp
)

target_link_libraries(
    embed_cxx
    wasmedge_shared
    fibonacci
)

get_filename_component(WASI_SDK_HOME $ENV{WASI_SDK_HOME} ABSOLUTE)
message("WASI_SDK_HOME=${WASI_SDK_HOME}")
# download wasi-sdk
if (NOT EXISTS ${WASI_SDK_HOME})
    set(WASI_SDK_VERSION 14.0)
    set(WASI_SDK_PATH ${CMAKE_CURRENT_BINARY_DIR}/deps/wasi-sdk)
    set(WASI_SDK_HOME ${WASI_SDK_PATH}/wasi-sdk-${WASI_SDK_VERSION})

    if(NOT EXISTS "${WASI_SDK_HOME}/bin/clang")
        message("download wasi-sdk")
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(WASI_SDK_TARGET "darwin")
        elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            set(WASI_SDK_TARGET "linux")
        elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            set(WASI_SDK_TARGET "mingw")
        endif ()

        set(WASI_SDK_TARGZ_URL https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-14/wasi-sdk-${WASI_SDK_VERSION}-${WASI_SDK_TARGET}.tar.gz)
        set(WASI_SDK_TARGZ_PATH ${CMAKE_CURRENT_BINARY_DIR}/wasi-sdk.tar.gz)
        file(DOWNLOAD ${WASI_SDK_TARGZ_URL} ${WASI_SDK_TARGZ_PATH} INACTIVITY_TIMEOUT 10)
        file(ARCHIVE_EXTRACT INPUT ${WASI_SDK_TARGZ_PATH} DESTINATION ${WASI_SDK_PATH})
    endif()
endif()

include(ExternalProject)
ExternalProject_Add(
        script_wasm
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/script
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/script_wasm
        CMAKE_CACHE_ARGS
        -DWASI_SDK_PREFIX:FILEPATH=${WASI_SDK_HOME}
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${WASI_SDK_HOME}/share/cmake/wasi-sdk.cmake
        -DCMAKE_SYSROOT:FILEPATH=${WASI_SDK_HOME}/share/wasi-sysroot
        -DWASM_OUT_DIR:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}
        INSTALL_COMMAND ""
        BUILD_ALWAYS 1
)

add_dependencies(
    embed_cxx
    script_wasm
)
