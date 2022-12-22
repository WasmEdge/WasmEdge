# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePluginWasmEdgeProcess
  SHARED
  processenv.cpp
  processfunc.cpp
  processmodule.cpp
)

target_compile_options(wasmedgePluginWasmEdgeProcess
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginWasmEdgeProcess
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginWasmEdgeProcess
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeProcess
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeProcess
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginWasmEdgeProcess DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
