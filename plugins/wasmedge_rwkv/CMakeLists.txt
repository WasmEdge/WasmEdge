cmake_minimum_required(VERSION 3.13)

add_library(wasmedgePluginRWKV SHARED
    rwkv.cpp
    rwkv.h
    rwkv_plugin.cpp
    ggml/src/ggml.c
    ggml/src/ggml-alloc.c
    ggml/src/ggml-quants.c
)

target_include_directories(wasmedgePluginRWKV
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/ggml/include
        ${CMAKE_CURRENT_SOURCE_DIR}/ggml/src
        ${WASMEDGE_PROJECT_BINARY_DIR}/include
        ${WASMEDGE_PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${WASMEDGE_PROJECT_BINARY_DIR}/_deps/spdlog-src/include
        ${WASMEDGE_PROJECT_BINARY_DIR}/_deps/fmt-src/include
)

target_compile_definitions(wasmedgePluginRWKV
    PRIVATE
        RWKV_BUILD
        RWKV_SHARED
        WASMEDGE_PLUGIN
        _FILE_OFFSET_BITS=64
)

target_compile_features(wasmedgePluginRWKV PRIVATE cxx_std_17)
target_compile_options(wasmedgePluginRWKV PRIVATE -Wno-error)

# Ensure fmt and spdlog dependencies are built
add_dependencies(wasmedgePluginRWKV fmt spdlog)

target_link_libraries(wasmedgePluginRWKV
    PRIVATE
        wasmedgeCAPI
        fmt::fmt
        spdlog::spdlog
)

set_target_properties(wasmedgePluginRWKV PROPERTIES
    PUBLIC_HEADER "rwkv.h"
)

install(TARGETS wasmedgePluginRWKV
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# -------------------------------------------------------------
# Test: test_rwkv
# -------------------------------------------------------------
enable_testing()
add_executable(test_rwkv
    test/test_rwkv.cpp
)

target_include_directories(test_rwkv
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${WASMEDGE_PROJECT_BINARY_DIR}/include
        ${WASMEDGE_PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${WASMEDGE_PROJECT_BINARY_DIR}/_deps/spdlog-src/include
        ${WASMEDGE_PROJECT_BINARY_DIR}/_deps/fmt-src/include
)

target_link_libraries(test_rwkv
    PRIVATE
        wasmedgePluginRWKV
        wasmedgeCAPI
        fmt::fmt
        spdlog::spdlog
)

target_compile_features(test_rwkv PRIVATE cxx_std_17)
add_test(NAME RWKVTest COMMAND test_rwkv)
