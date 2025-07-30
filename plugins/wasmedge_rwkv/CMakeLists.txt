# Minimum CMake version and project name
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
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ggml/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ggml/src
    ${WASMEDGE_PROJECT_SOURCE_DIR}/thirdparty/blake3
    ${WASMEDGE_PROJECT_BINARY_DIR}/include
    ${WASMEDGE_PROJECT_SOURCE_DIR}/include
)

# Define the plugin macro
target_compile_definitions(wasmedgePluginRWKV
  PRIVATE
    RWKV_BUILD
    RWKV_SHARED
)

# Set standard and warning flags
target_compile_features(wasmedgePluginRWKV PRIVATE cxx_std_17)
target_compile_options(wasmedgePluginRWKV PRIVATE -Wno-error)

# Link required WasmEdge libraries and fmt
target_link_libraries(wasmedgePluginRWKV
  PRIVATE
    wasmedgePlugin
    fmt::fmt            # 👈 Ensures fmt headers are found and linked
)

# Install the plugin
install(TARGETS wasmedgePluginRWKV DESTINATION lib)
