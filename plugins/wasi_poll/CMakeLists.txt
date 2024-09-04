# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_plugin(wasmedgePluginWasiPoll
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_WASI_POLL}
  SOURCES
  env.cpp
  func.cpp
  module.cpp
)

target_include_directories(wasmedgePluginWasiPoll
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty
)

install(
  TARGETS wasmedgePluginWasiPoll
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
