# Minimum CMake version
cmake_minimum_required(VERSION 3.13)

# Define the plugin target
add_library(wasmedgePluginRWKV SHARED
  rwkv.cpp
  rwkv.h
  rwkv_plugin.cpp
  ggml/src/ggml.c
  ggml/src/ggml-alloc.c
  ggml/src/ggml-quants.c
)

# Set include directories
target_include_directories(wasmedgePluginRWKV
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ggml/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ggml/src
    ${WASMEDGE_PROJECT_SOURCE_DIR}/thirdparty/blake3
    ${WASMEDGE_PROJECT_BINARY_DIR}/include
    ${WASMEDGE_PROJECT_SOURCE_DIR}/include
  PRIVATE
    ${WASMEDGE_PROJECT_BINARY_DIR}/_deps/spdlog-src/include
    ${WASMEDGE_PROJECT_BINARY_DIR}/_deps/fmt-src/include
)

# Define the plugin macro and enable plugin-specific build flags
target_compile_definitions(wasmedgePluginRWKV
  PRIVATE
    RWKV_BUILD
    RWKV_SHARED
    WASMEDGE_PLUGIN
)

# Set standard and warning flags
target_compile_features(wasmedgePluginRWKV PRIVATE cxx_std_17)
target_compile_options(wasmedgePluginRWKV PRIVATE -Wno-error)

# Link required WasmEdge libraries and fmt
target_link_libraries(wasmedgePluginRWKV
  PRIVATE
    wasmedgeCAPI
    fmt::fmt
)

# Install the plugin
install(TARGETS wasmedgePluginRWKV DESTINATION lib)
