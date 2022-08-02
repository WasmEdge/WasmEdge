# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePluginHttpsReq
  SHARED
  httpsreqenv.cpp
  httpsreqfunc.cpp
  httpsreqmodule.cpp
)

target_compile_options(wasmedgePluginHttpsReq
  PUBLIC
  -DWASMEDGE_PLUGIN
)


if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginHttpsReq
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginHttpsReq
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginHttpsReq
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  -L/usr/lib -lssl -lcrypto
)

install(TARGETS wasmedgePluginHttpsReq DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
