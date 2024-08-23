# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgePluginWasmEdgeTensorflow
  SHARED
  tensorflow_env.cpp
  tensorflow_func.cpp
  tensorflow_module.cpp
)

target_compile_options(wasmedgePluginWasmEdgeTensorflow
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasmEdgeTensorflow
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeTensorflow
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeTensorflow
    PRIVATE
    wasmedge_shared
  )
endif()

include(WASINNDeps)
wasmedge_setup_tf_target(wasmedgePluginWasmEdgeTensorflow)

install(
  TARGETS wasmedgePluginWasmEdgeTensorflow
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
