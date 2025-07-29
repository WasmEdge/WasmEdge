# Minimum CMake version and project name
cmake_minimum_required(VERSION 3.16)
project(wasmedgePluginRWKV)

# Ensure 64-bit file offsets for rwkv.cpp
add_definitions(-D_FILE_OFFSET_BITS=64)

# --------------------------------------
# RWKV + GGML source files
# --------------------------------------

# Core RWKV implementation and plugin entry point
set(RWKV_SOURCES
  rwkv.cpp
  rwkv_plugin.cpp
)

# GGML: all C files in the submodule (using GLOB for CMake 3.16 compatibility)
file(GLOB GGML_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/ggml/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/ggml/src/*.c"
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
  ${CMAKE_SOURCE_DIR}/include         # WasmEdge headers (for wasmedge/Plugin.h)
  ${CMAKE_BINARY_DIR}/include         # Generated WasmEdge headers
  ${CMAKE_CURRENT_SOURCE_DIR}         # RWKV plugin headers
  ${CMAKE_CURRENT_SOURCE_DIR}/ggml/include  # GGML headers
  ${CMAKE_CURRENT_SOURCE_DIR}/ggml/src      # GGML implementation headers
  ${CMAKE_SOURCE_DIR}/thirdparty/blake3     # BLAKE3 headers (if needed)
)

# --------------------------------------
# Compiler definitions for symbol export
# --------------------------------------

target_compile_definitions(wasmedgePluginRWKV PRIVATE
  RWKV_SHARED
  RWKV_BUILD
)

# Compiler flags to disable -Werror for this target
target_compile_options(wasmedgePluginRWKV PRIVATE
  -Wno-error
)

# --------------------------------------
# Link with WasmEdge C API and BLAKE3 (if needed)
# --------------------------------------

target_link_libraries(wasmedgePluginRWKV PRIVATE
  wasmedge_c
  utilBlake3  # Add if BLAKE3 is required
)

# --------------------------------------
# Installation path (for WasmEdge to load plugin)
# --------------------------------------

include(GNUInstallDirs)
install(TARGETS wasmedgePluginRWKV
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
        