# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeValidator
  formchecker.cpp
  validator.cpp
)

target_compile_options(wasmedgeValidator
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeValidator
  PUBLIC
  wasmedgeCommon
  wasmedgeAST
)
