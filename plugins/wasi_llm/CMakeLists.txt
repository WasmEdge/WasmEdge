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

wasmedge_add_plugin(wasmedgePluginWasiLLM
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_LLM}
  SOURCES
  wasillmfunc.cpp
  wasillmmodule.cpp
  wasillmenv.cpp
)

target_include_directories(wasmedgePluginWasiLLM
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginWasiLLM PRIVATE
  train_gpt2_cpu
)

install(
  TARGETS wasmedgePluginWasiLLM
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
