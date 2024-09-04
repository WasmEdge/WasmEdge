# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_plugin(wasmedgePluginWasmEdgeProcess
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_PROCESS}
  SOURCES
  processenv.cpp
  processfunc.cpp
  processmodule.cpp
)

target_include_directories(wasmedgePluginWasmEdgeProcess
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

install(
  TARGETS wasmedgePluginWasmEdgeProcess
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
