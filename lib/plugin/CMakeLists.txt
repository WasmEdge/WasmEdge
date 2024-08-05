# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

add_subdirectory(wasi_logging)

wasmedge_add_library(wasmedgePlugin
  plugin.cpp
)

target_include_directories(wasmedgePlugin
  PRIVATE
  ${PROJECT_BINARY_DIR}/include/api
  ${PROJECT_SOURCE_DIR}/include/api
)

target_link_libraries(wasmedgePlugin
  PUBLIC
  wasmedgePluginWasiLogging
  wasmedgeCommon
  wasmedgeLoaderFileMgr
  wasmedgePO
)
