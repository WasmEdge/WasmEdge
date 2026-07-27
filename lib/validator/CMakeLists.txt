# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_library(wasmedgeValidator
  formchecker.cpp
  validator.cpp
  component_context.cpp
  component_validator.cpp
  component_name.cpp
)

target_link_libraries(wasmedgeValidator
  PUBLIC
  wasmedgeCommon
)
