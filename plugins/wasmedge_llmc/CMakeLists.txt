# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgePluginWasmEdgeLLMC
  SHARED
  llmc_func.cpp
  llmc_module.cpp
  llmc_env.cpp
)

option(WASMEDGE_PLUGIN_LLMC_CUDA "Training GPT2 with CUDA" OFF)

message(STATUS "Start fetching llm.c source")
include(FetchContent)

if (WASMEDGE_PLUGIN_LLMC_CUDA)
  set(CUDALIB ON)
  message(STATUS "Build wasmedge_llmc with CUDA backend")
else()
  message(STATUS "Build wasmedge_llmc with CPU backend")
endif()

FetchContent_Declare(
  llmc
  GIT_REPOSITORY https://github.com/WasmEdge/llm.c
)
FetchContent_MakeAvailable(llmc)

if (WASMEDGE_PLUGIN_LLMC_CUDA)
  target_link_libraries(wasmedgePluginWasmEdgeLLMC PRIVATE
    train_gpt2_cuda
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeLLMC PRIVATE
    train_gpt2_cpu
  )
endif()

target_compile_options(wasmedgePluginWasmEdgeLLMC
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasmEdgeLLMC
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeLLMC
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeLLMC
    PRIVATE
    wasmedge_shared
  )
endif()

install(
  TARGETS wasmedgePluginWasmEdgeLLMC
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
