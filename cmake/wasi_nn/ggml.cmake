# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

include(FetchContent)

# Function for preparing the llama dependency.
function(wasmedge_setup_llama_target target)
  if(NOT TARGET llama)
    # llama.cpp options
    # Disable warnings and debug messages
    set(LLAMA_ALL_WARNINGS OFF)
    # Disable curl dependency
    set(LLAMA_CURL OFF)
    set(LLAMA_METAL_NDEBUG ON)
    set(LLAMA_BUILD_COMMON ON)
    set(LLAMA_BUILD_TOOLS ON)
    set(GGML_ACCELERATE OFF)
    set(GGML_AMX OFF)
    set(GGML_OPENMP OFF)
    set(BUILD_SHARED_LIBS OFF)

    if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_NATIVE)
      message(STATUS "WASI-NN GGML LLAMA backend: Enable GGML_NATIVE(AVX/AVX2/FMA/F16C)")
      set(GGML_NATIVE ON)
    else()
      message(STATUS "WASI-NN GGML LLAMA backend: Disable GGML_NATIVE(AVX/AVX2/FMA/F16C)")
      set(GGML_NATIVE OFF)
      set(GGML_AVX OFF)
      set(GGML_AVX2 OFF)
      set(GGML_FMA OFF)
      set(GGML_F16C OFF)
    endif()

    if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS)
      message(STATUS "WASI-NN GGML LLAMA backend: Enable GGML_CUDA")
      set(GGML_CUDA ON)
      # We need to set GGML_USE_CUDA for clip from llava.
      add_compile_definitions(GGML_USE_CUDA)
    else()
      message(STATUS "WASI-NN GGML LLAMA backend: Disable GGML_CUDA")
      set(GGML_CUDA OFF)
    endif()

    if (WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS)
      message(STATUS "WASI-NN GGML LLAMA backend: Enable GGML_BLAS")
      set(GGML_BLAS ON)
      set(GGML_BLAS_VENDOR "OpenBLAS")
    else()
      message(STATUS "WASI-NN GGML LLAMA backend: Disable GGML_BLAS")
      set(GGML_BLAS OFF)
    endif()

    if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_METAL)
      message(STATUS "WASI-NN GGML LLAMA backend: Enable GGML_METAL")
      set(GGML_METAL ON)
      set(GGML_METAL_EMBED_LIBRARY ON)
    else()
      message(STATUS "WASI-NN GGML LLAMA backend: Disable GGML_METAL")
      set(GGML_METAL OFF)
    endif()

    # setup llama.cpp
    message(STATUS "Downloading llama.cpp source")
    include(FetchContent)
    FetchContent_Declare(
      llama
      GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
      GIT_TAG        a29e4c0b7b23e020107058480dabbe03b7cba6e1  # b8757
      GIT_SHALLOW    FALSE
    )
    FetchContent_MakeAvailable(llama)
    message(STATUS "Downloading llama.cpp source -- done")
    set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml-base PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml-cpu PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET llama PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET mtmd PROPERTY POSITION_INDEPENDENT_CODE ON)
    if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS)
      set_property(TARGET ggml-cuda PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
  endif()
  # Ignore unused function warnings in common.h in llama.cpp.
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${target}
      PRIVATE
      -Wno-error=unused-function
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target}
      PRIVATE
      -Wno-error=unused-function
      -Wno-error=implicit-float-conversion
      -Wno-error=documentation
      -Wno-error=unused-template
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${target}
      PRIVATE
      /wd4305
      /wd4244
    )
  endif()
  # Only the plugin library needs to fully link the dependency.
  if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
    wasmedge_setup_simdjson()
    target_link_libraries(${target}
      PRIVATE
      common
      simdjson::simdjson
      mtmd
    )
  endif()
endfunction()
