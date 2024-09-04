# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

find_package(ZLIB REQUIRED)

set(ZLIB_COMPAT ON)

wasmedge_add_plugin(wasmedgePluginWasmEdgeZlib
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_ZLIB}
  SOURCES
  zlibenv.cpp
  zlibfunc.cpp
  zlibmodule.cpp
)

target_include_directories(wasmedgePluginWasmEdgeZlib
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginWasmEdgeZlib
  PRIVATE
  z
)

install(
  TARGETS wasmedgePluginWasmEdgeZlib
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
