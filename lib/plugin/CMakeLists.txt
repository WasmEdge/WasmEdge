# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if(WASMEDGE_PLUGIN_WASI_LOGGING)
  add_subdirectory(wasi_logging)
endif()

wasmedge_add_library(wasmedgePlugin
  plugin.cpp
)

target_include_directories(wasmedgePlugin
  PRIVATE
  ${PROJECT_BINARY_DIR}/include/api
  ${PROJECT_SOURCE_DIR}/include/api
)

target_link_libraries(wasmedgePlugin
  PUBLIC
  # BUILTIN-PLUGIN: Temporarily add the wasi-logging plugin here until the new
  # plugin architecture is ready in 0.15.0.
  $<$<BOOL:${WASMEDGE_PLUGIN_WASI_LOGGING}>:wasmedgePluginWasiLogging>
  wasmedgeCommon
  wasmedgeLoaderFileMgr
  wasmedgePO
)
if(WASMEDGE_PLUGIN_WASI_LOGGING)
  target_compile_definitions(wasmedgePlugin PRIVATE WASMEDGE_PLUGIN_WASI_LOGGING)
endif()
