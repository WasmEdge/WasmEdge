# Minimum CMake version and project name
cmake_minimum_required(VERSION 3.16)
project(wasmedgePluginRWKV)

# --------------------------------------
# RWKV + GGML source files
# --------------------------------------

# Core RWKV implementation and plugin entry point
set(RWKV_SOURCES
  rwkv.cpp
  rwkv_plugin.cpp
)

# GGML: all C files in the submodule (recursive to include all .c files in subdirectories)
file(GLOB_RECURSE GGML_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/ggml/*.c"
)

# --------------------------------------
# Create shared library for WasmEdge plugin
# --------------------------------------

add_library(wasmedgePluginRWKV SHARED
  ${RWKV_SOURCES}
  ${GGML_SOURCES}
)

# --------------------------------------
# Include paths
# --------------------------------------

target_include_directories(wasmedgePluginRWKV PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/ggml/include  # Updated include path here
  ${CMAKE_CURRENT_SOURCE_DIR}/ggml/src      # Added include path for ggml-impl.h
)

# --------------------------------------
# Compiler definitions for symbol export
# --------------------------------------

target_compile_definitions(wasmedgePluginRWKV PRIVATE
  RWKV_SHARED
  RWKV_BUILD
)

# --------------------------------------
# Link with WasmEdge C API
# Adjust to `wasmedge` if `wasmedge_c` isn't found
# --------------------------------------

target_link_libraries(wasmedgePluginRWKV PRIVATE
  wasmedge_c
)

# --------------------------------------
# Installation path (for WasmEdge to load plugin)
# --------------------------------------

include(GNUInstallDirs)
install(TARGETS wasmedgePluginRWKV
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
