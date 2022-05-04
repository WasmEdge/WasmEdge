# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePlugin
  plugin.cpp
)

target_link_libraries(wasmedgePlugin
  PUBLIC
  wasmedgeCommon
  wasmedgeLoaderFileMgr
  wasmedgePO
)
