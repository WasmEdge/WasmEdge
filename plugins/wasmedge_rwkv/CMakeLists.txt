# Build the wasmedgePluginRWKV as a shared library
add_library(wasmedgePluginRWKV SHARED
  rwkv.cpp           # Core RWKV implementation
  rwkv_plugin.cpp    # WasmEdge plugin wrapper
)

# Include directories: current source dir and optional thirdparty headers
target_include_directories(wasmedgePluginRWKV PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/thirdparty/rwkv     # Optional if using ggml headers
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
