# Minimum CMake version and project name
cmake_minimum_required(VERSION 3.16)
project(wasmedgePluginRWKV)

# Ensure 64-bit file offsets for rwkv.cpp
add_definitions(-D_FILE_OFFSET_BITS=64)

# Debug: Print include paths for verification
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "WasmEdge include path: D:/Programs/wasmedge/WasmEdge/include")

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
  D:/Programs/wasmedge/WasmEdge/include  # Explicit WasmEdge headers path
  ${CMAKE_BINARY_DIR}/include            # Generated WasmEdge headers
  ${CMAKE_CURRENT_SOURCE_DIR}            # RWKV plugin headers
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

# --------------------------------------
# Compiler flags
# --------------------------------------

# Disable -Werror for this target
target_compile_options(wasmedgePluginRWKV PRIVATE
  -Wno-error
)

# --------------------------------------
# Link with WasmEdge C API and BLAKE3
# --------------------------------------

# Link against wasmedge (adjust target name based on WasmEdge project)
target_link_libraries(wasmedgePluginRWKV PRIVATE
  wasmedge  # Use wasmedge instead of wasmedge_c
  utilBlake3
)

# --------------------------------------
# Installation path (for WasmEdge to load plugin)
# --------------------------------------

include(GNUInstallDirs)
install(TARGETS wasmedgePluginRWKV
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
