# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_library(wasmedgePluginWasiNN
  SHARED
  wasinnenv.cpp
  wasinnfunc.cpp
  wasinnmodule.cpp
  wasinn_openvino.cpp
  wasinn_openvino_genai.cpp
  wasinn_onnx.cpp
  wasinn_tf.cpp
  wasinn_torch.cpp
  wasinn_tfl.cpp
  GGML/core/ggml_core.cpp
  wasinn_neuralspeed.cpp
  wasinn_piper.cpp
  wasinn_whisper.cpp
  wasinn_chattts.cpp
  wasinn_mlx.cpp
  wasinn_bitnet.cpp
  wasinn_sigmastar_ipu.cpp
  sigs_qwen2.cpp
)

include(WASINNDeps)
wasmedge_setup_wasinn_target(wasmedgePluginWasiNN PLUGINLIB)

set(WASMEDGE_WASI_NN_VERSION "0.1.34" CACHE STRING "WasmEdge WASI-NN library version")
set(WASMEDGE_WASI_NN_SOVERSION "0" CACHE STRING "WasmEdge WASI-NN library soversion")

# Handle the version of the WASI-NN plugin
string(REPLACE "." ";" WASI_NN_VERSION_LIST ${WASMEDGE_WASI_NN_VERSION})
list(GET WASI_NN_VERSION_LIST 0 WASI_NN_VERSION_MAJOR)
list(GET WASI_NN_VERSION_LIST 1 WASI_NN_VERSION_MINOR)
list(GET WASI_NN_VERSION_LIST 2 WASI_NN_VERSION_PATCH)

target_compile_definitions(wasmedgePluginWasiNN PRIVATE
    WASI_NN_VERSION_MAJOR=${WASI_NN_VERSION_MAJOR}
    WASI_NN_VERSION_MINOR=${WASI_NN_VERSION_MINOR}
    WASI_NN_VERSION_PATCH=${WASI_NN_VERSION_PATCH}
)
# This foreach iteration handles the additional sources.
# The dependencies are moved to `cmake/WASINNDeps.cmake`.
foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
  string(TOLOWER ${BACKEND} BACKEND)
  if(BACKEND STREQUAL "mlx")
    target_sources(wasmedgePluginWasiNN
      PRIVATE
      MLX/prompt/prompt.cpp
      MLX/model/llm/transformer.cpp
      MLX/model/llm/registry.cpp
      MLX/model/gemma3/language.cpp
      MLX/model/gemma3/vision.cpp
      MLX/model/gemma3/gemma3.cpp
      MLX/model/converter.cpp
      MLX/model/utils.cpp
      MLX/model/vlm_base.cpp
      MLX/model/vlm_sampling.cpp
      MLX/model/whisper/whisper.cpp
      MLX/model/whisper/tokenizer.cpp
      MLX/model/whisper/decoding.cpp
      MLX/model/whisper_transcribe.cpp
      MLX/mlx/base.cpp
      MLX/mlx/linear.cpp
      MLX/mlx/convolution.cpp
      MLX/mlx/positional_encoding.cpp
      MLX/mlx/activations.cpp
      MLX/mlx/embedding.cpp
      MLX/mlx/normalization.cpp
      MLX/mlx/transformer.cpp
      MLX/mlx/pooling.cpp
      MLX/mlx/quantized.cpp
    )
  endif()

  if(BACKEND STREQUAL "ggml")
    target_sources(wasmedgePluginWasiNN
      PRIVATE
      GGML/core/ggml_core.cpp
      GGML/core/input_processor.cpp
      GGML/core/output_generator.cpp
      GGML/metadata/metadata_parser.cpp
      GGML/compute/compute_engine.cpp
      GGML/compute/inference_manager.cpp
      GGML/tts/tts_core.cpp
      GGML/utils.cpp
    )
    if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP)
      find_package(hip REQUIRED)
      find_package(hipblas REQUIRED)
      set(GGML_HIP ON CACHE BOOL "Build GGML with HIP" FORCE)
      if(DEFINED ENV{GPU_TARGETS})
        set(GPU_TARGETS $ENV{GPU_TARGETS} CACHE STRING "HIP GPU targets")
      else()
        set(GPU_TARGETS "gfx90c" CACHE STRING "Default for Vega iGPU")
      endif()
      message(STATUS "Enabling HIP for ggml backend with targets: ${GPU_TARGETS}")
      target_compile_definitions(wasmedgePluginWasiNN PRIVATE GGML_HIP=${GGML_HIP} GPU_TARGETS=${GPU_TARGETS})
      target_link_libraries(wasmedgePluginWasiNN PRIVATE hip::host roc::hipblas)
    endif()
  endif()

  # SigmaStar IPU backend: link sgs_transformers + MI runtime (vendor SDK).
  if(BACKEND STREQUAL "sigmastar_ipu")
    if(NOT DEFINED SGS_LIB_DIR OR NOT DEFINED SGS_INCLUDE_DIR OR NOT DEFINED SGS_SDK_INCLUDE_DIR)
      message(FATAL_ERROR "sigmastar_ipu backend requires -DSGS_LIB_DIR, -DSGS_INCLUDE_DIR, -DSGS_SDK_INCLUDE_DIR")
    endif()
    target_include_directories(wasmedgePluginWasiNN PRIVATE
      ${SGS_INCLUDE_DIR}
      ${SGS_SDK_INCLUDE_DIR}
    )
    target_link_libraries(wasmedgePluginWasiNN PRIVATE
      ${SGS_LIB_DIR}/libsgs_transformers.a
      ${SGS_LIB_DIR}/libutils_npy.a
      ${SGS_LIB_DIR}/libutils_buffer.a
      ${SGS_LIB_DIR}/libutils_common.a
      ${SGS_LIB_DIR}/libsgsalgo_sampler.a
      ${SGS_LIB_DIR}/libsgsalgo_processor.a
    )
    target_link_directories(wasmedgePluginWasiNN PRIVATE ${SIGMASTAR_MI_LIB_DIR})
    target_link_libraries(wasmedgePluginWasiNN PRIVATE
      mi_ipu mi_sys mi_common cam_os_wrapper cam_fs_wrapper stdc++fs pthread
    )
  endif()
endforeach()

target_compile_options(wasmedgePluginWasiNN
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasiNN
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_BUILD_WASI_NN_RPC)
  add_definitions(-DWASMEDGE_BUILD_WASI_NN_RPC)
  target_include_directories(wasmedgePluginWasiNN
    SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR}
  )
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasiNNRPC
  )
endif()

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasmedge_shared
  )
endif()

install(
  TARGETS wasmedgePluginWasiNN
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
  COMPONENT WasmEdge
)
