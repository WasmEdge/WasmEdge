# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePluginWasiThreads
  SHARED
  threadfunc.cpp
  threadmodule.cpp
  threadenv.cpp
)

target_compile_options(wasmedgePluginWasiThreads
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasiThreads
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginWasiThreads
  PUBLIC
  wasmedgeExecutor
  wasmedgeCommon
  wasmedgeSystem
  wasmedgePlugin
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasiThreads
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasiThreads
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginWasiThreads DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
