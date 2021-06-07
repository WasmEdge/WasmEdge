# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeValidator
  formchecker.cpp
  validator.cpp
)

target_link_libraries(wasmedgeValidator
  PUBLIC
  wasmedgeCommon
  wasmedgeAST
)
