# Build the wasmedgePluginRWKV as a shared library
cmake_minimum_required(VERSION 3.16)
project(wasmedgePluginRWKV)

# RWKV core source files
set(RWKV_SOURCES
  rwkv.cpp           # Core RWKV implementation
  rwkv_plugin.cpp    # WasmEdge plugin wrapper
)

# Add all ggml C source files
file(GLOB GGML_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/ggml/*.c"
)

add_library(wasmedgePluginRWKV SHARED
  ${RWKV_SOURCES}
  ${GGML_SOURCES}
)

# Include directories: current source dir and ggml headers
target_include_directories(wasmedgePluginRWKV PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/ggml
)

# Define visibility for Windows/Linux shared builds
target_compile_definitions(wasmedgePluginRWKV PRIVATE
  RWKV_SHARED
  RWKV_BUILD
)

# Link with WasmEdge C API
target_link_libraries(wasmedgePluginRWKV PRIVATE
  wasmedge_c   # Change to wasmedge if you're not using wasmedge_c
)

# Install to standard WasmEdge plugin directory
install(TARGETS wasmedgePluginRWKV
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
