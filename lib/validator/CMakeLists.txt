# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgeValidator
  formchecker.cpp
  validator.cpp
)

target_link_libraries(wasmedgeValidator
  PUBLIC
  wasmedgeCommon
)
