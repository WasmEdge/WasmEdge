# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeValidator
  formchecker.cpp
  validator.cpp
)

target_link_libraries(wasmedgeValidator
  PRIVATE
  wasmedgeCommon
)

target_include_directories(wasmedgeValidator
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
