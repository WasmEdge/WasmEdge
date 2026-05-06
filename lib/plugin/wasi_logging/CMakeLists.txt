# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# BUILTIN-PLUGIN: Temporarily move the wasi-logging plugin sources here until
# the new plugin architecture is ready in 0.15.0.

wasmedge_add_library(wasmedgePluginWasiLogging
  func.cpp
  module.cpp
)

target_link_libraries(wasmedgePluginWasiLogging
  PUBLIC
  wasmedgeCommon
)
