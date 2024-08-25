# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgePluginWasmEdgeTensorflowLite
  SHARED
  tensorflowlite_env.cpp
  tensorflowlite_func.cpp
  tensorflowlite_module.cpp
)

target_compile_options(wasmedgePluginWasmEdgeTensorflowLite
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasmEdgeTensorflowLite
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeTensorflowLite
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeTensorflowLite
    PRIVATE
    wasmedge_shared
  )
endif()

include(WASINNDeps)
wasmedge_setup_tflite_target(wasmedgePluginWasmEdgeTensorflowLite)

install(
  TARGETS wasmedgePluginWasmEdgeTensorflowLite
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
