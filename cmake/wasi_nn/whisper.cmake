# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

include(FetchContent)

# Function for preparing the whisper dependency.
function(wasmedge_setup_whisper_target target)
  if(NOT TARGET whisper)
    # setup whisper
    FetchContent_Declare(
      whisper
      GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
      GIT_TAG        69339af2d104802f3f201fd419163defba52890e
      GIT_SHALLOW    FALSE
    )
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Whisper not build shared")
    set(GGML_OPENMP OFF)
    set(GGML_ACCELERATE OFF)
    set(GGML_BLAS OFF)
    if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND WASMEDGE_PLUGIN_WASI_NN_WHISPER_METAL)
      message(STATUS "WASI-NN Whisper backend: Enable GGML_METAL")
      set(GGML_METAL ON)
      set(GGML_METAL_EMBED_LIBRARY ON)
    else()
      message(STATUS "WASI-NN Whisper backend: Disable GGML_METAL")
      set(GGML_METAL OFF)
    endif()
    if(WASMEDGE_PLUGIN_WASI_NN_WHISPER_CUDA)
      message(STATUS "WASI-NN Whisper backend: Enable GGML_CUDA")
      set(GGML_CUDA ON)
    else()
      message(STATUS "WASI-NN Whisper backend: Disable GGML_CUDA")
      set(GGML_CUDA OFF)
    endif()
    FetchContent_MakeAvailable(whisper)
    set_property(TARGET whisper PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_include_directories(whisper
      PUBLIC
      ${whisper_SOURCE_DIR}
    )
  endif()
  # Only the plugin library needs to fully link the dependency.
  if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
    wasmedge_setup_simdjson()
    target_link_libraries(${target}
      PRIVATE
      whisper
      simdjson::simdjson
    )
  endif()
endfunction()
