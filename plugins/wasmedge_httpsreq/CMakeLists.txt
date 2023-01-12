# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

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
  OpenSSL::Crypto
  OpenSSL::SSL
)
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginHttpsReq
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginHttpsReq
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginHttpsReq DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
