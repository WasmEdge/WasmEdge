# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_library(wasmedgePO
  argument_parser.cpp
)

target_link_libraries(wasmedgePO
  PUBLIC
  wasmedgeCommon
)
