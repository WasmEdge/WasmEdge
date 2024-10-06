# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgePO
  argument_parser.cpp
)

target_link_libraries(wasmedgePO
  PUBLIC
  wasmedgeCommon
)
