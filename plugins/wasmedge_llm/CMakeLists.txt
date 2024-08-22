# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# TODO: Fetch llm.c source.

message(STATUS "Start fetching llm.c source")
include(FetchContent)
FetchContent_Declare(
  llmc
  GIT_REPOSITORY https://github.com/WasmEdge/llm.c
)
FetchContent_MakeAvailable(llmc)

wasmedge_add_plugin(wasmedgePluginWasmEdgeLLM
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_LLM}
  SOURCES
  llm_func.cpp
  llm_module.cpp
  llm_env.cpp
)

target_include_directories(wasmedgePluginWasmEdgeLLM
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginWasmEdgeLLM PRIVATE
  train_gpt2_cpu
)

install(
  TARGETS wasmedgePluginWasmEdgeLLM
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
