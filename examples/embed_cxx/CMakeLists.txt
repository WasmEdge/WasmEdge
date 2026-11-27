cmake_minimum_required(VERSION 3.18)

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

if (DEFINED ENV{WASI_SDK_HOME})
    get_filename_component(WASI_SDK_HOME "$ENV{WASI_SDK_HOME}" ABSOLUTE)
endif()
message("WASI_SDK_HOME=${WASI_SDK_HOME}")
# download wasi-sdk
if (NOT WASI_SDK_HOME OR NOT EXISTS "${WASI_SDK_HOME}")
    set(WASI_SDK_VERSION 34.0)
    set(WASI_SDK_PATH ${CMAKE_CURRENT_BINARY_DIR}/deps/wasi-sdk)

    if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
        set(WASI_SDK_OS "macos")
    elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
        set(WASI_SDK_OS "linux")
    elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
        set(WASI_SDK_OS "windows")
    endif ()
    if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
        set(WASI_SDK_ARCH "x86_64")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
        set(WASI_SDK_ARCH "arm64")
    endif ()
    if (NOT WASI_SDK_OS OR NOT WASI_SDK_ARCH)
        message(FATAL_ERROR "No prebuilt wasi-sdk for ${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}, please set WASI_SDK_HOME")
    endif ()
    set(WASI_SDK_TARGET ${WASI_SDK_ARCH}-${WASI_SDK_OS})
    set(WASI_SDK_SHA256_x86_64-linux b761e3a0721dbae9c09a0059e5fdb2bf917d1b4a8a7b430fb3b5aafb0984b2c4)
    set(WASI_SDK_SHA256_arm64-linux f7e243dff54d60bcc576e94d6166b69f410f2500ae4a9ceef34315be10e77971)
    set(WASI_SDK_SHA256_x86_64-macos 87d27fa8adc68dee59bfbf2e22a6d34ef717c34d6bf1d8af2a56fc929d9ce0eb)
    set(WASI_SDK_SHA256_arm64-macos 9c59398106b417f8f14913380fdf0097a8cc0ff4af9eb3ce0065a859e88d49e9)
    set(WASI_SDK_SHA256_x86_64-windows cccb5c323a9b34f0349a9b09e8804a0a7632c68c3310f4b5f437ed57d7e71d8f)
    set(WASI_SDK_SHA256_arm64-windows 45e1c71f3e965621e7b98ebe1d37b0e4b1f77f3e8072113ffb4534e67b1a4b7c)
    set(WASI_SDK_HOME ${WASI_SDK_PATH}/wasi-sdk-${WASI_SDK_VERSION}-${WASI_SDK_TARGET})

    if(NOT EXISTS "${WASI_SDK_HOME}/bin/clang")
        message("download wasi-sdk")
        set(WASI_SDK_TARGZ_URL https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-34/wasi-sdk-${WASI_SDK_VERSION}-${WASI_SDK_TARGET}.tar.gz)
        set(WASI_SDK_TARGZ_PATH ${CMAKE_CURRENT_BINARY_DIR}/wasi-sdk.tar.gz)
        file(DOWNLOAD ${WASI_SDK_TARGZ_URL} ${WASI_SDK_TARGZ_PATH} INACTIVITY_TIMEOUT 10
            EXPECTED_HASH SHA256=${WASI_SDK_SHA256_${WASI_SDK_TARGET}})
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
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${WASI_SDK_HOME}/share/cmake/wasi-sdk-p1.cmake
        -DCMAKE_SYSROOT:FILEPATH=${WASI_SDK_HOME}/share/wasi-sysroot
        -DWASM_OUT_DIR:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}
        INSTALL_COMMAND ""
        BUILD_ALWAYS 1
)

add_dependencies(
    embed_cxx
    script_wasm
)
