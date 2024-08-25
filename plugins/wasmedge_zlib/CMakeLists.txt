# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

find_package(ZLIB REQUIRED)

set(ZLIB_COMPAT ON)

wasmedge_add_library(wasmedgePluginWasmEdgeZlib
  SHARED
  zlibenv.cpp
  zlibfunc.cpp
  zlibmodule.cpp
)

target_compile_options(wasmedgePluginWasmEdgeZlib
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasmEdgeZlib
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeZlib
    PRIVATE
    wasmedgeCAPI
    z
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeZlib
    PRIVATE
    wasmedge_shared
    z
  )
endif()

install(
  TARGETS wasmedgePluginWasmEdgeZlib
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
