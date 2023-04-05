# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(ZLIB REQUIRED)

wasmedge_add_library(wasmedgePluginZlib
  SHARED
  zlibenv.cpp
  zlibfunc.cpp
  zlibmodule.cpp
)

target_compile_options(wasmedgePluginZlib
  PUBLIC
  -DWASMEDGE_PLUGIN
)


if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginZlib
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginZlib
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

#  ${PROJECT_SOURCE_DIR}/thirdparty

target_link_libraries(wasmedgePluginZlib
  PUBLIC
  ZLIB::ZLIB
)
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginZlib
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginZlib
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginZlib DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
