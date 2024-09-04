# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_plugin(wasmedgePluginWasmEdgeTensorflow
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_TENSORFLOW}
  SOURCES
  tensorflow_env.cpp
  tensorflow_func.cpp
  tensorflow_module.cpp
)

target_include_directories(wasmedgePluginWasmEdgeTensorflow
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

include(WASINNDeps)
wasmedge_setup_tf_target(wasmedgePluginWasmEdgeTensorflow)

install(
  TARGETS wasmedgePluginWasmEdgeTensorflow
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
