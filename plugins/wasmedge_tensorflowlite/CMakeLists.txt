# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_plugin(wasmedgePluginWasmEdgeTensorflowLite
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_TENSORFLOWLITE}
  SOURCES
  tensorflowlite_env.cpp
  tensorflowlite_func.cpp
  tensorflowlite_module.cpp
)

target_include_directories(wasmedgePluginWasmEdgeTensorflowLite
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

include(WASINNDeps)
wasmedge_setup_tflite_target(wasmedgePluginWasmEdgeTensorflowLite)

install(
  TARGETS wasmedgePluginWasmEdgeTensorflowLite
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
