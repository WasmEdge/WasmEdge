# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# TODO: Fetch llm.c source.

wasmedge_add_library(wasmedgePluginLLMC
  SHARED
  llmc_func.cpp
  llmc_module.cpp
  llmc_env.cpp
)

message(STATUS "Start fetching llm.c source")
include(FetchContent)
FetchContent_Declare(
  llmc
  GIT_REPOSITORY https://github.com/WasmEdge/llm.c
)
FetchContent_MakeAvailable(llmc)

target_link_libraries(wasmedgePluginLLMC PRIVATE
  train_gpt2_cpu
)

target_compile_options(wasmedgePluginLLMC
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginLLMC
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginLLMC
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginLLMC
    PRIVATE
    wasmedge_shared
  )
endif()

install(
  TARGETS wasmedgePluginLLMC
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
