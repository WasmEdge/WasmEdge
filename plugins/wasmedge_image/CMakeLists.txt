# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgePluginWasmEdgeImage
  SHARED
  image_env.cpp
  image_func.cpp
  image_module.cpp
)

target_compile_options(wasmedgePluginWasmEdgeImage
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasmEdgeImage
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Need libjpeg, libpng, zlib, and boost.
find_package(ZLIB REQUIRED)
wasmedge_setup_jpeg()
wasmedge_setup_png()
wasmedge_setup_boost()

target_link_libraries(wasmedgePluginWasmEdgeImage
  PUBLIC
  Boost::boost
  wasmedgeDepsJPEG
  wasmedgeDepsPNG
  z
)
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeImage
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeImage
    PRIVATE
    wasmedge_shared
  )
endif()

install(
  TARGETS wasmedgePluginWasmEdgeImage
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
  COMPONENT WasmEdge
)
