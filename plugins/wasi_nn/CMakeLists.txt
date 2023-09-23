# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# llama.cpp options
set(LLAMA_ALL_WARNINGS OFF)
if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS)
  message(INFO "WASI-NN GGML LLAMA backend: Enable LLAMA_BLAS")
  # Default use OpenBLAS
  set(LLAMA_BLAS ON)
  set(LLAMA_BLAS_VENDOR "OpenBLAS")
else()
  message(INFO "WASI-NN GGML LLAMA backend: Disable LLAMA_BLAS")
  set(LLAMA_BLAS OFF)
endif()

add_subdirectory(thirdparty)

wasmedge_add_library(wasmedgePluginWasiNN
  SHARED
  wasinnenv.cpp
  wasinnfunc.cpp
  wasinnmodule.cpp
  openvino.cpp
  onnx.cpp
  tf.cpp
  torch.cpp
  tfl.cpp
  ggml.cpp
)

target_compile_options(wasmedgePluginWasiNN
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasiNN
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty/ggml
)

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

string(TOLOWER ${WASMEDGE_PLUGIN_WASI_NN_BACKEND} BACKEND)
if(BACKEND STREQUAL "ggml")
  target_link_libraries(wasmedgePluginWasiNN PRIVATE llama)
endif()

include(WASINNDeps)
wasmedge_setup_wasinn_target(wasmedgePluginWasiNN)

install(TARGETS wasmedgePluginWasiNN DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
