# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8)
FetchContent_MakeAvailable(cpr)

wasmedge_add_plugin(wasmedgePluginWasiHttp
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_WASI_HTTP}
  SOURCES
  env.cpp
  func.cpp
  module.cpp
)

target_include_directories(wasmedgePluginWasiHttp
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgePluginWasiHttp
  PUBLIC
  cpr::cpr
)

install(
  TARGETS wasmedgePluginWasiHttp
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
