# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgeValidator
  component_validator.cpp
  formchecker.cpp
  validator.cpp
)

target_link_libraries(wasmedgeValidator
  PUBLIC
  wasmedgeCommon
)
