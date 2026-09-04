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
    # Disable openssl dependency
    set(LLAMA_OPENSSL OFF)
    set(LLAMA_METAL_NDEBUG ON)
    set(LLAMA_BUILD_COMMON ON)
    # Build the mtmd library only instead of the whole llama.cpp tools tree
    set(LLAMA_BUILD_TOOLS OFF)
    set(LLAMA_BUILD_MTMD ON)
    # Disable the ffmpeg-based video input support in mtmd
    set(MTMD_VIDEO OFF CACHE BOOL "enable video support in mtmd" FORCE)
    set(GGML_ACCELERATE OFF)
    set(GGML_AMX OFF)
    set(GGML_OPENMP OFF)
    set(BUILD_SHARED_LIBS OFF)
    # All fetched static libraries are linked into the shared plugin library.
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
      GIT_TAG        918bca552078be4b3437f93117f542ea39972f5f  # v0.3.0
      GIT_SHALLOW    FALSE
    )
    FetchContent_MakeAvailable(llama)
    message(STATUS "Downloading llama.cpp source -- done")
    # Reach llama.cpp's headers through -isystem / -external:I. They are pulled
    # into this plugin's own translation units, so without this they compile
    # under WASMEDGE_CFLAGS and trip -Werror on clang-cl, where -Wall means
    # -Weverything.
    foreach(LLAMA_TARGET IN ITEMS
        llama-common ggml ggml-base ggml-cpu ggml-cuda llama mtmd)
      wasmedge_mark_system_includes(${LLAMA_TARGET})
    endforeach()
  endif()
  # Ignore unused function warnings in common.h in llama.cpp.
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${target}
      PRIVATE
      -Wno-error=unused-function
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # The GGML sources index vectors with int, which clang-cl reports under
    # -Weverything. Keep the diagnostics, but do not fail the build on them.
    target_compile_options(${target}
      PRIVATE
      -Wno-error=unused-function
      -Wno-error=implicit-float-conversion
      -Wno-error=documentation
      -Wno-error=unused-template
      -Wno-error=sign-conversion
      -Wno-error=extra-semi-stmt
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
      llama-common
      simdjson::simdjson
      mtmd
    )
  endif()
endfunction()
