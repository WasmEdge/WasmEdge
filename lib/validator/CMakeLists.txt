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

target_include_directories(wasmedgeValidator
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
