# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2023 Second State INC

wasmedge_add_library(wasmedgePluginWasmEdgeOpenCVMini
  SHARED
  opencvmini_env.cpp
  opencvmini_func.cpp
  opencvmini_module.cpp
)

target_compile_options(wasmedgePluginWasmEdgeOpenCVMini
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginWasmEdgeOpenCVMini
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginWasmEdgeOpenCVMini
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeOpenCVMini
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeOpenCVMini
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginWasmEdgeOpenCVMini DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
