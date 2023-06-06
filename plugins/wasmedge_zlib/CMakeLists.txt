# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# Don't reply on System zlib
# find_package(ZLIB REQUIRED)

set(ZLIB_COMPAT ON)
set(ZLIBNG_ENABLE_TESTS OFF)

FetchContent_Declare(
    ZLIB
    GIT_REPOSITORY "https://github.com/zlib-ng/zlib-ng.git"
    GIT_TAG 2.0.7
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(ZLIB)

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
    ZLIB
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeZlib
    PRIVATE
    wasmedge_shared
    ZLIB
  )
endif()

install(TARGETS wasmedgePluginWasmEdgeZlib DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
