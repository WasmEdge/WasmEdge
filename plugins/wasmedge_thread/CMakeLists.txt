# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePluginWasmEdgeThread
  SHARED
  threadenv.cpp
  threadfunc.cpp
  threadmodule.cpp
)

target_compile_options(wasmedgePluginWasmEdgeThread
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginWasmEdgeThread
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginWasmEdgeThread
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgePluginWasmEdgeThread
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)

install(TARGETS wasmedgePluginWasmEdgeThread DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)

wasmedge_add_library(wasmedgeHostModuleWasmEdgeThread
  threadenv.cpp
  threadfunc.cpp
  threadmodule.cpp
)

target_include_directories(wasmedgeHostModuleWasmEdgeThread
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgeHostModuleWasmEdgeThread
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  wasmedgePlugin
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_options(wasmedgeHostModuleWasmEdgeThread
    PUBLIC
    -u_ZN8WasmEdge4Host6WasmEdgeThread17WasmEdgeThreadEnvironment8RegisterE
  )
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgeHostModuleWasmEdgeThread
    PUBLIC
    -u__ZN8WasmEdge4Host6WasmEdgeThread17WasmEdgeThreadEnvironment8RegisterE
  )
endif()
