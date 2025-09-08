# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  wasinn_ggml.cpp
  wasinn_neuralspeed.cpp
  wasinn_piper.cpp
  wasinn_whisper.cpp
  wasinn_chattts.cpp
  wasinn_mlx.cpp
  wasinn_bitnet.cpp
)

include(WASINNDeps)
wasmedge_setup_wasinn_target(wasmedgePluginWasiNN PLUGINLIB)

set(WASMEDGE_WASI_NN_VERSION "0.1.32" CACHE STRING "WasmEdge WASI-NN library version")
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

# This for-each iteration is for the additional sources.
# The dependencies are moved into `cmake/WASINNDeps.cmake`.
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
